﻿
import {
	Logger, logger,
	LoggingDebugSession,
	InitializedEvent, TerminatedEvent, StoppedEvent, BreakpointEvent, OutputEvent,
	ProgressStartEvent, ProgressUpdateEvent, ProgressEndEvent, InvalidatedEvent,
	Thread, StackFrame, Scope, Source, Handles, Breakpoint, MemoryEvent,
	ThreadEvent,
	DebugSession
} from '@vscode/debugadapter';
import { DebugProtocol } from '@vscode/debugprotocol';
import { basename } from 'path-browserify';
import { XLangRuntime, IRuntimeBreakpoint,RuntimeVariable, timeout, IRuntimeVariableType } from './xLangRuntime';
import { Subject } from 'await-notify';
import * as base64 from 'base64-js';
import * as vscode from 'vscode';
import * as path from 'path';
import * as net from 'net';
import * as cp from 'child_process';

const ipPortRegex = /^((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2}):([0-9]|[1-9]\d|[1-9]\d{2}|[1-9]\d{3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$/;
const ipRegex = /^((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})$/;
const ipHostPortRegex = /^(?:(?:\d{1,3}\.){3}\d{1,3}|\[(?:[a-fA-F0-9:]+)\]|(?:[a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+|\w+):\d{1,5}$/;

/**
 * This interface describes the xLang specific launch attributes
 * (which are not part of the Debug Adapter Protocol).
 * The schema for these attributes lives in the package.json of the xLang extension.
 * The interface should always match this schema.
 */
interface ILaunchRequestArguments extends DebugProtocol.LaunchRequestArguments {
	/** An absolute path to the "program" to debug. */
	program: string;
	/** Automatically stop target after launch. If not specified, target does not stop. */
	stopOnEntry?: boolean;
	/** enable logging the Debug Adapter Protocol */
	trace?: boolean;
	/** run without debugging */
	noDebug?: boolean;
	/** if specified, results in a simulated compile error in launch. */
	compileError?: 'default' | 'show' | 'hide';
}

interface IAttachRequestArguments extends ILaunchRequestArguments { }


export class XLangDebugSession extends LoggingDebugSession {

	// a XLang runtime (or debugger)
	private _runtime: XLangRuntime;

	private _variableHandles = new Handles<'locals' | 'globals'| RuntimeVariable>();

	private _configurationDone = new Subject();

	private _xlangStarted = new Subject();

	private _cancellationTokens = new Map<number, boolean>();

	private _reportProgress = false;
	private _progressId = 10000;
	private _cancelledProgressId: string | undefined = undefined;
	private _isProgressCancellable = true;

	private _valuesInHex = false;
	private _useInvalidatedEvent = false;

	private _addressesInHex = true;

	private _isLaunch = true;

	private _mapFrameIdThreadId : Map<Number, Number> = new Map();

	private _srcList : string[] = [];

	private _xlangProcess;

	public getRuntime(){
		return this._runtime;
	}
	/**
	 * Creates a new debug adapter that is used for one debug session.
	 * We configure the default implementation of a debug adapter here.
	 */
	public constructor() {
		super("xLang.txt");

		// this debugger uses zero-based lines and columns
		this.setDebuggerLinesStartAt1(false);
		this.setDebuggerColumnsStartAt1(false);

		this._runtime = new XLangRuntime();

		// setup event handlers
		this._runtime.on('threadStarted', (threadID) => {
			this.sendEvent(new ThreadEvent('started', threadID));
		});
		this._runtime.on('threadExited', (threadID) => {
			this.sendEvent(new ThreadEvent('exited', threadID));
		});
		this._runtime.on('stopOnEntry', (threadID) => {
			this.sendEvent(new StoppedEvent('entry', threadID));
		});
		this._runtime.on('stopOnStep', (threadID) => {
			this.sendEvent(new StoppedEvent('step', threadID));
		});
		this._runtime.on('stopOnBreakpoint', (threadID) => {
			this.sendEvent(new StoppedEvent('breakpoint', threadID));
		});
		this._runtime.on('stopOnDataBreakpoint', (threadID) => {
			this.sendEvent(new StoppedEvent('data breakpoint', threadID));
		});
		this._runtime.on('stopOnInstructionBreakpoint', (threadID) => {
			this.sendEvent(new StoppedEvent('instruction breakpoint', threadID));
		});
		this._runtime.on('stopOnException', (exception, threadID) => {
			if (exception) {
				this.sendEvent(new StoppedEvent(`exception(${exception})`, threadID));
			} else {
				this.sendEvent(new StoppedEvent('exception', threadID));
			}
		});
		this._runtime.on('breakpointValidated', (bp: IRuntimeBreakpoint) => {
			this.sendEvent(new BreakpointEvent('changed', { verified: bp.verified, id: bp.id } as DebugProtocol.Breakpoint));
		});
		this._runtime.on('output', (type, text, filePath, line, column, threadID) => {
			let category: string;
			switch(type) {
				case 'prio': category = 'important'; break;
				case 'out': category = 'stdout'; break;
				case 'err': category = 'stderr'; break;
				default: category = 'console'; break;
			}
			const e: DebugProtocol.OutputEvent = new OutputEvent(`${text}\n`, category);

			if (text === 'start' || text === 'startCollapsed' || text === 'end') {
				e.body.group = text;
				e.body.output = `group-${text}\n`;
			}

			e.body.source = this.createSource(filePath);
			e.body.line = this.convertDebuggerLineToClient(line);
			e.body.column = this.convertDebuggerColumnToClient(column);
			this.sendEvent(e);
		});
		this._runtime.on('end', () => {
			this.sendEvent(new TerminatedEvent());
		});
		this._runtime.on('breakpointState', (path, line) => {
			path = path.replaceAll('/', '\\');
			let srcIdx = this._srcList.indexOf(path);
			if (line >= 100000){ // failed
				line -= 100000; 
				this.sendEvent(new BreakpointEvent('changed', {verified: false, id: srcIdx * 10000000 + line * 1000}));
			}else{
				this.sendEvent(new BreakpointEvent('changed', {verified: true,  id: srcIdx * 10000000 + line * 1000}));
			}
		});
		this._runtime.on('xlangStarted', (started) => {
			this._xlangStarted.notifyValue = started;
			this._xlangStarted.notify();
		});
	}

	/**
	 * The 'initialize' request is the first request called by the frontend
	 * to interrogate the features the debug adapter provides.
	 */
	protected initializeRequest(response: DebugProtocol.InitializeResponse, args: DebugProtocol.InitializeRequestArguments): void {

		if (args.supportsProgressReporting) {
			this._reportProgress = true;
		}
		if (args.supportsInvalidatedEvent) {
			this._useInvalidatedEvent = true;
		}

		// build and return the capabilities of this debug adapter:
		response.body = response.body || {};

		// the adapter implements the configurationDone request.
		response.body.supportsConfigurationDoneRequest = true;

		// make VS Code use 'evaluate' when hovering over source
		response.body.supportsEvaluateForHovers = true;

		// make VS Code show a 'step back' button
		response.body.supportsStepBack = true;

		// make VS Code support data breakpoints
		response.body.supportsDataBreakpoints = true;

		// make VS Code support completion in REPL
		response.body.supportsCompletionsRequest = true;
		response.body.completionTriggerCharacters = [ ".", "[" ];

		// make VS Code send cancel request
		response.body.supportsCancelRequest = true;

		// make VS Code send the breakpointLocations request
		response.body.supportsBreakpointLocationsRequest = false;

		// make VS Code provide "Step in Target" functionality
		response.body.supportsStepInTargetsRequest = true;

		// the adapter defines two exceptions filters, one with support for conditions.
		response.body.supportsExceptionFilterOptions = true;
		response.body.exceptionBreakpointFilters = [
			{
				filter: 'namedException',
				label: "Named Exception",
				description: `Break on named exceptions. Enter the exception's name as the Condition.`,
				default: false,
				supportsCondition: true,
				conditionDescription: `Enter the exception's name`
			},
			{
				filter: 'otherExceptions',
				label: "Other Exceptions",
				description: 'This is a other exception',
				default: true,
				supportsCondition: false
			}
		];

		// make VS Code send exceptionInfo request
		response.body.supportsExceptionInfoRequest = true;

		// make VS Code send setVariable request
		response.body.supportsSetVariable = true;

		// make VS Code send setExpression request
		response.body.supportsSetExpression = true;

		// make VS Code send disassemble request
		response.body.supportsDisassembleRequest = true;
		response.body.supportsSteppingGranularity = true;
		response.body.supportsInstructionBreakpoints = true;

		// make VS Code able to read and write variable memory
		response.body.supportsReadMemoryRequest = true;
		response.body.supportsWriteMemoryRequest = true;

		response.body.supportSuspendDebuggee = true;
		response.body.supportTerminateDebuggee = true;
		response.body.supportsFunctionBreakpoints = true;
		// request all stack frame at once
		response.body.supportsDelayedStackTraceLoading = false;

		this.sendResponse(response);

		// since this debug adapter can accept configuration requests like 'setBreakpoint' at any time,
		// we request them early by sending an 'initializeRequest' to the frontend.
		// The frontend will end the configuration sequence by calling 'configurationDone' request.
		//this.sendEvent(new InitializedEvent());
	}

	/**
	 * Called at the end of the configuration sequence.
	 * Indicates that all breakpoints etc. have been sent to the DA and that the 'launch' can start.
	 */
	protected configurationDoneRequest(response: DebugProtocol.ConfigurationDoneResponse, args: DebugProtocol.ConfigurationDoneArguments): void {
		super.configurationDoneRequest(response, args);

		// notify the launchRequest that configuration has finished
		this._configurationDone.notify();
	}

	protected disconnectRequest(response: DebugProtocol.DisconnectResponse, args: DebugProtocol.DisconnectArguments, request?: DebugProtocol.Request): void {
		console.log(`disconnectRequest suspend: ${args.suspendDebuggee}, terminate: ${args.terminateDebuggee}`);
		if (this._isLaunch)
		{
			this._runtime.close();
		}
		this.sendResponse(response);
	}

	protected async attachRequest(response: DebugProtocol.AttachResponse, args: IAttachRequestArguments) {
		this._isLaunch = false;
		//get running module key and source file from runtime
		//return this.launchRequest(response, args);
	}

	protected async launchRequest(response: DebugProtocol.LaunchResponse, args: ILaunchRequestArguments) {
		let runXlang = true;
		let dbgAddr = await vscode.window.showInputBox({value: "localhost:3142", prompt: "input remote xlang dbg address and port, empty or cancel to run xlang on a random port locally", placeHolder: "ip or host name:port"});
		if (dbgAddr)
		{
			dbgAddr = dbgAddr.replace(/\s/g, '');
			if(ipHostPortRegex.test(dbgAddr))
			{
				const strList = dbgAddr.split(":");
				this._runtime.serverAddress = strList[0];
				this._runtime.serverPort = Number(strList[1]);
				runXlang = false;
			}
			else
			{
				await vscode.window.showErrorMessage("please input address and port, debugging stopped", { modal: true }, "ok");
				this.sendResponse(response);
				this.sendEvent(new TerminatedEvent());
				return;
			}
		}
		if (runXlang)
		{
			let port = await this.getValidPort();
			let xlangBin = vscode.workspace.getConfiguration('XLangDebugger').get<string>('ExePath');
			this._xlangProcess = cp.spawn(xlangBin, ['-event_loop', '-dbg', '-enable_python', `-port ${port}`], { shell: true, detached: true });
			this._runtime.serverAddress = "localhost";
			this._runtime.serverPort = port;
		}
		
		this._runtime.checkStarted();
		await this._xlangStarted.wait();
		if (!this._xlangStarted.notifyValue)
		{
			await vscode.window.showErrorMessage(`can not connect to a xlang dbg server at ${this._runtime.serverAddress}:${this._runtime.serverPort} debugging stopped`, { modal: true }, "ok");
			this.sendResponse(response);
			this.sendEvent(new TerminatedEvent());
			return;
		}

		this._isLaunch = true;
		// make sure to 'Stop' the buffered logging if 'trace' is not set
		logger.setup(args.trace ? Logger.LogLevel.Verbose : Logger.LogLevel.Stop, false);
		
		await this._runtime.loadSource(args.program);
		this.sendEvent(new InitializedEvent());

		// wait 1 second until configuration has finished (and configurationDoneRequest has been called)
		await this._configurationDone.wait();

		args.stopOnEntry = true;
		
		// start the program in the runtime
		await this._runtime.start(!!args.stopOnEntry, !args.noDebug);

		if (args.compileError) {
			// simulate a compile/build error in "launch" request:
			// the error should not result in a modal dialog since 'showUser' is set to false.
			// A missing 'showUser' should result in a modal dialog.
			this.sendErrorResponse(response, {
				id: 1001,
				format: `compile error: some fake error.`,
				showUser: args.compileError === 'show' ? true : (args.compileError === 'hide' ? false : undefined)
			});
		} else {
			this.sendResponse(response);
		}
	}

	protected setFunctionBreakPointsRequest(response: DebugProtocol.SetFunctionBreakpointsResponse, args: DebugProtocol.SetFunctionBreakpointsArguments, request?: DebugProtocol.Request): void {
		this.sendResponse(response);
	}

	protected async setBreakPointsRequest(response: DebugProtocol.SetBreakpointsResponse, args: DebugProtocol.SetBreakpointsArguments): Promise<void> {
		const path = (args.source.path as string).toLowerCase();
		const clientLines = args.lines || [];
		
		let srcIdx = this._srcList.indexOf(path);
		if ( srcIdx < 0){
			this._srcList.push(path);
			srcIdx = this._srcList.length - 1;
		}
		
		this._runtime.setBreakPoints(path, clientLines, (lines) => {
			let actualBreakpoints = lines.map(l => {
				if (l >= 200000){
					l -= 200000;
					//let ret = new Breakpoint(false, l, undefined, new Source(path, path));
					let ret = new Breakpoint(false, l);
					ret.reason = 'pending';
					ret.setId(srcIdx * 10000000 + l * 1000); // ret.setId(srcIdx * 10000000 + l * 1000 + c);
					return ret;
				}
				else if (l >= 100000){
					l -= 100000;
					let ret = new Breakpoint(false, l);
					ret.reason = 'failed';
					ret.setId(srcIdx * 10000000 +  + l * 1000);
					return ret;
				}
				else{
					let ret = new Breakpoint(true, l);
					ret.setId(srcIdx * 10000000 +  + l * 1000);
					return ret;
				}
			});
			response.body = {
				breakpoints: actualBreakpoints
			};
			this.sendResponse(response);
		});
	}

	protected breakpointLocationsRequest(response: DebugProtocol.BreakpointLocationsResponse, args: DebugProtocol.BreakpointLocationsArguments, request?: DebugProtocol.Request): void {
		if (args.source.path) {
			const bps = this._runtime.getBreakpoints(args.source.path, this.convertClientLineToDebugger(args.line));
			response.body = {
				breakpoints: bps.map(col => {
					return {
						line: args.line,
						column: this.convertDebuggerColumnToClient(col)
					};
				})
			};
		} else {
			response.body = {
				breakpoints: []
			};
		}
		this.sendResponse(response);
	}

	protected async setExceptionBreakPointsRequest(response: DebugProtocol.SetExceptionBreakpointsResponse, args: DebugProtocol.SetExceptionBreakpointsArguments): Promise<void> {
		let namedException: string | undefined = undefined;
		let otherExceptions = false;

		if (args.filterOptions) {
			for (const filterOption of args.filterOptions) {
				switch (filterOption.filterId) {
					case 'namedException':
						namedException = args.filterOptions[0].condition;
						break;
					case 'otherExceptions':
						otherExceptions = true;
						break;
				}
			}
		}

		if (args.filters) {
			if (args.filters.indexOf('otherExceptions') >= 0) {
				otherExceptions = true;
			}
		}

		this._runtime.setExceptionsFilters(namedException, otherExceptions);

		this.sendResponse(response);
	}

	protected exceptionInfoRequest(response: DebugProtocol.ExceptionInfoResponse, args: DebugProtocol.ExceptionInfoArguments) {
		response.body = {
			exceptionId: 'Exception ID',
			description: 'This is a descriptive description of the exception.',
			breakMode: 'always',
			details: {
				message: 'Message contained in the exception.',
				typeName: 'Short type name of the exception object',
				stackTrace: 'stack frame 1\nstack frame 2',
			}
		};
		this.sendResponse(response);
	}

	protected threadsRequest(response: DebugProtocol.ThreadsResponse){
		this._runtime.getThreads((threads) => {
			
			let tids: Number[] = [];
			response.body = {
				threads: threads.map((t) => {
					tids.push(t.id);
					const thread: DebugProtocol.Thread = new Thread(t.id, t.name);
					return thread;
				}),
		};

			this._mapFrameIdThreadId.forEach((value, key) => {
				if (!tids.includes(value)){
					this._mapFrameIdThreadId.delete(key);
				}
			});

		this.sendResponse(response);
        });
	}

	protected stackTraceRequest(response: DebugProtocol.StackTraceResponse, args: DebugProtocol.StackTraceArguments): void {
		const threadId = args.threadId;
		const startFrame = typeof args.startFrame === 'number' ? args.startFrame : 0;
		const maxLevels = typeof args.levels === 'number' ? args.levels : 1000;
		const endFrame = startFrame + maxLevels;

		this._mapFrameIdThreadId.forEach((value, key) => {
			if (value === threadId){
				this._mapFrameIdThreadId.delete(key);
			}
		});

		this._runtime.stack(threadId, startFrame, endFrame, (stk) => {
			response.body = {
				stackFrames: stk.frames.map((f, ix) => {
					this._mapFrameIdThreadId.set(f.id, threadId);
					const sf: DebugProtocol.StackFrame = new StackFrame(f.id, f.name, this.createSource(f.file), this.convertDebuggerLineToClient(f.line));
					if (typeof f.column === 'number') {
						sf.column = this.convertDebuggerColumnToClient(f.column);
					}
					if (typeof f.instruction === 'number') {
						const address = this.formatAddress(f.instruction);
						sf.name = `${f.name} ${address}`;
						sf.instructionPointerReference = address;
					}
					this._runtime.currentLine = sf.line;
					return sf;
				}),
				// 4 options for 'totalFrames':
				//omit totalFrames property: 	// VS Code has to probe/guess. Should result in a max. of two requests
				totalFrames: stk.count			// stk.count is the correct size, should result in a max. of two requests
				//totalFrames: 1000000 			// not the correct size, should result in a max. of two requests
				//totalFrames: endFrame + 20 	// dynamically increases the size with every requested chunk, results in paging
			};
			this.sendResponse(response);
        });
	}

	protected scopesRequest(response: DebugProtocol.ScopesResponse, args: DebugProtocol.ScopesArguments): void {
		response.body = {
			scopes: [
				new Scope("Locals", this._runtime.createScopeRef('locals',args.frameId,null,null), false),
				new Scope("Globals", this._runtime.createScopeRef('globals', args.frameId,null,null), true)
			]
		};
		this.sendResponse(response);
	}

	protected async writeMemoryRequest(response: DebugProtocol.WriteMemoryResponse, { data, memoryReference, offset = 0 }: DebugProtocol.WriteMemoryArguments) {
		const variable = this._variableHandles.get(Number(memoryReference));
		if (typeof variable === 'object') {
			const decoded = base64.toByteArray(data);
			//TODO:variable.setMemory(decoded, offset);
			response.body = { bytesWritten: decoded.length };
		} else {
			response.body = { bytesWritten: 0 };
		}

		this.sendResponse(response);
		this.sendEvent(new InvalidatedEvent(['variables']));
	}

	protected async readMemoryRequest(response: DebugProtocol.ReadMemoryResponse, { offset = 0, count, memoryReference }: DebugProtocol.ReadMemoryArguments) {
		const variable = this._variableHandles.get(Number(memoryReference));
		if (typeof variable === 'object' && variable.memory) {
			const memory = variable.memory.subarray(
				Math.min(offset, variable.memory.length),
				Math.min(offset + count, variable.memory.length),
			);

			response.body = {
				address: offset.toString(),
				data: base64.fromByteArray(memory),
				unreadableBytes: count - memory.length
			};
		} else {
			response.body = {
				address: offset.toString(),
				data: '',
				unreadableBytes: count
			};
		}

		this.sendResponse(response);
	}

	protected async variablesRequest(response: DebugProtocol.VariablesResponse, args: DebugProtocol.VariablesArguments, request?: DebugProtocol.Request): Promise<void> {
		let cb = (vs: RuntimeVariable[]) => {
			response.body = {
				variables: vs.map(v => this.convertFromRuntime(v))
			};
			this.sendResponse(response);
		};

		const v = this._runtime.getScopeRef(args.variablesReference);
		const varType = v[0];
		const frameId = v[1];
		const objId = v[3];
		const threadId = this._mapFrameIdThreadId.get(frameId);
		if (varType === 'locals') {
			this._runtime.getLocalVariables(threadId, frameId,cb);
		} else if (varType === 'globals') {
			this._runtime.getGlobalVariables(threadId, cb);
		} else {
			this._runtime.getObject(threadId, frameId,varType,objId,
				args.start===undefined?0:args.start,
				args.count===undefined?-1:args.count,
				cb);
		}
	}

	protected setVariableRequest(response: DebugProtocol.SetVariableResponse, args: DebugProtocol.SetVariableArguments): void {
		let cb = (v: RuntimeVariable) => {
			response.body = this.convertFromRuntime(v);
			this.sendResponse(response);
		};
		const varInfo = this._runtime.getScopeRef(args.variablesReference);
		const varType = varInfo[0];
		const frameId = varInfo[1];
		const objId = varInfo[3];
		const threadId = this._mapFrameIdThreadId.get(frameId);
		this._runtime.setObject(threadId, frameId,varType,objId,args.name,args.value,cb);
	}

	protected continueRequest(response: DebugProtocol.ContinueResponse, args: DebugProtocol.ContinueArguments): void {
		this._runtime.continue(false, args.threadId, () => {
			this.sendResponse(response);
		});
	}

	protected reverseContinueRequest(response: DebugProtocol.ReverseContinueResponse, args: DebugProtocol.ReverseContinueArguments): void {
		this._runtime.continue(false, args.threadId, () => {
			this.sendResponse(response);
		});
	}

	protected nextRequest(response: DebugProtocol.NextResponse, args: DebugProtocol.NextArguments): void {
		this._runtime.step(args.granularity === 'instruction', false, args.threadId,
			() => {
				this.sendResponse(response);
			});
	}

	protected stepBackRequest(response: DebugProtocol.StepBackResponse, args: DebugProtocol.StepBackArguments): void {
		this._runtime.step(args.granularity === 'instruction', true, args.threadId,
		() => {
		this.sendResponse(response);
		});
	}

	protected stepInTargetsRequest(response: DebugProtocol.StepInTargetsResponse, args: DebugProtocol.StepInTargetsArguments) {
		const targets = this._runtime.getStepInTargets(args.frameId);
		response.body = {
			targets: targets.map(t => {
				return { id: t.id, label: t.label };
			})
		};
		this.sendResponse(response);
	}

	protected stepInRequest(response: DebugProtocol.StepInResponse, args: DebugProtocol.StepInArguments): void {
		this._runtime.stepIn(args.threadId, args.targetId, () => {
			this.sendResponse(response);
		});
	}

	protected stepOutRequest(response: DebugProtocol.StepOutResponse, args: DebugProtocol.StepOutArguments): void {
		this._runtime.stepOut(args.threadId, () => {
			this.sendResponse(response);
		});
	}

	protected async evaluateRequest(response: DebugProtocol.EvaluateResponse, args: DebugProtocol.EvaluateArguments): Promise<void> {
		let reply: string | undefined;
		let rv: RuntimeVariable | undefined;

		switch (args.context) {
			case 'repl':
				// handle some REPL commands:
				// 'evaluate' supports to create and delete breakpoints from the 'repl':
				const matches = /new +([0-9]+)/.exec(args.expression);
				if (matches && matches.length === 2) {
					const mbp = await this._runtime.setBreakPoint(this._runtime.sourceFile, this.convertClientLineToDebugger(parseInt(matches[1])));
					const bp = new Breakpoint(mbp.verified, this.convertDebuggerLineToClient(mbp.line), undefined, this.createSource(this._runtime.sourceFile)) as DebugProtocol.Breakpoint;
					bp.id= mbp.id;
					this.sendEvent(new BreakpointEvent('new', bp));
					reply = `breakpoint created`;
				} else {
					const matches = /del +([0-9]+)/.exec(args.expression);
					if (matches && matches.length === 2) {
						const mbp = this._runtime.clearBreakPoint(this._runtime.sourceFile, this.convertClientLineToDebugger(parseInt(matches[1])));
						if (mbp) {
							const bp = new Breakpoint(false) as DebugProtocol.Breakpoint;
							bp.id= mbp.id;
							this.sendEvent(new BreakpointEvent('removed', bp));
							reply = `breakpoint deleted`;
						}
					} else {
						const matches = /progress/.exec(args.expression);
						if (matches && matches.length === 1) {
							if (this._reportProgress) {
								reply = `progress started`;
								this.progressSequence();
							} else {
								reply = `frontend doesn't support progress (capability 'supportsProgressReporting' not set)`;
							}
						}
					}
				}
				// fall through

			default:
				if (args.expression.startsWith('$')) {
					rv = this._runtime.getLocalVariable(args.expression.substr(1));
				} else {
					rv = new RuntimeVariable('eval', this.convertToRuntime(args.expression));
				}
				break;
		}

		if (rv) {
			const v = this.convertFromRuntime(rv);
			response.body = {
				result: v.value,
				type: v.type,
				variablesReference: v.variablesReference,
				presentationHint: v.presentationHint
			};
		} else {
			response.body = {
				result: reply ? reply : `evaluate(context: '${args.context}', '${args.expression}')`,
				variablesReference: 0
			};
		}

		this.sendResponse(response);
	}

	protected setExpressionRequest(response: DebugProtocol.SetExpressionResponse, args: DebugProtocol.SetExpressionArguments): void {
		if (args.expression.startsWith('$')) {
			const rv = this._runtime.getLocalVariable(args.expression.substr(1));
			if (rv) {
				rv.value = this.convertToRuntime(args.value);
				response.body = this.convertFromRuntime(rv);
				this.sendResponse(response);
			} else {
				this.sendErrorResponse(response, {
					id: 1002,
					format: `variable '{lexpr}' not found`,
					variables: { lexpr: args.expression },
					showUser: true
				});
			}
		} else {
			this.sendErrorResponse(response, {
				id: 1003,
				format: `'{lexpr}' not an assignable expression`,
				variables: { lexpr: args.expression },
				showUser: true
			});
		}
	}

	private async progressSequence() {

		const ID = '' + this._progressId++;

		await timeout(100);

		const title = this._isProgressCancellable ? 'Cancellable operation' : 'Long running operation';
		const startEvent: DebugProtocol.ProgressStartEvent = new ProgressStartEvent(ID, title);
		startEvent.body.cancellable = this._isProgressCancellable;
		this._isProgressCancellable = !this._isProgressCancellable;
		this.sendEvent(startEvent);
		this.sendEvent(new OutputEvent(`start progress: ${ID}\n`));

		let endMessage = 'progress ended';

		for (let i = 0; i < 100; i++) {
			await timeout(500);
			this.sendEvent(new ProgressUpdateEvent(ID, `progress: ${i}`));
			if (this._cancelledProgressId === ID) {
				endMessage = 'progress cancelled';
				this._cancelledProgressId = undefined;
				this.sendEvent(new OutputEvent(`cancel progress: ${ID}\n`));
				break;
			}
		}
		this.sendEvent(new ProgressEndEvent(ID, endMessage));
		this.sendEvent(new OutputEvent(`end progress: ${ID}\n`));

		this._cancelledProgressId = undefined;
	}

	protected dataBreakpointInfoRequest(response: DebugProtocol.DataBreakpointInfoResponse, args: DebugProtocol.DataBreakpointInfoArguments): void {
		response.body = {
            dataId: null,
            description: "cannot break on data access",
            accessTypes: undefined,
            canPersist: false
        };

		if (args.variablesReference && args.name) {
			const v = this._variableHandles.get(args.variablesReference);
			if (v === 'globals') {
				response.body.dataId = args.name;
				response.body.description = args.name;
				response.body.accessTypes = [ "write" ];
				response.body.canPersist = true;
			} else {
				response.body.dataId = args.name;
				response.body.description = args.name;
				response.body.accessTypes = ["read", "write", "readWrite"];
				response.body.canPersist = true;
			}
		}

		this.sendResponse(response);
	}

	protected setDataBreakpointsRequest(response: DebugProtocol.SetDataBreakpointsResponse, args: DebugProtocol.SetDataBreakpointsArguments): void {
		// clear all data breakpoints
		this._runtime.clearAllDataBreakpoints();

		response.body = {
			breakpoints: []
		};

		for (const dbp of args.breakpoints) {
			const ok = this._runtime.setDataBreakpoint(dbp.dataId, dbp.accessType || 'write');
			response.body.breakpoints.push({
				verified: ok
			});
		}

		this.sendResponse(response);
	}

	protected completionsRequest(response: DebugProtocol.CompletionsResponse, args: DebugProtocol.CompletionsArguments): void {
		response.body = {
			targets: [
				{
					label: "item 10",
					sortText: "10"
				},
				{
					label: "item 1",
					sortText: "01",
					detail: "detail 1"
				},
				{
					label: "item 2",
					sortText: "02",
					detail: "detail 2"
				},
				{
					label: "array[]",
					selectionStart: 6,
					sortText: "03"
				},
				{
					label: "func(arg)",
					selectionStart: 5,
					selectionLength: 3,
					sortText: "04"
				}
			]
		};
		this.sendResponse(response);
	}

	protected cancelRequest(response: DebugProtocol.CancelResponse, args: DebugProtocol.CancelArguments) {
		if (args.requestId) {
			this._cancellationTokens.set(args.requestId, true);
		}
		if (args.progressId) {
			this._cancelledProgressId= args.progressId;
		}
	}

	protected disassembleRequest(response: DebugProtocol.DisassembleResponse, args: DebugProtocol.DisassembleArguments) {
		const baseAddress = parseInt(args.memoryReference);
		const offset = args.instructionOffset || 0;
		const count = args.instructionCount;

		const isHex = args.memoryReference.startsWith('0x');
		const pad = isHex ? args.memoryReference.length-2 : args.memoryReference.length;

		const loc = this.createSource(this._runtime.sourceFile);

		let lastLine = -1;

		const instructions = this._runtime.disassemble(baseAddress+offset, count).map(instruction => {
			const address = instruction.address.toString(isHex ? 16 : 10).padStart(pad, '0');
			const instr : DebugProtocol.DisassembledInstruction = {
				address: isHex ? `0x${address}` : `${address}`,
				instruction: instruction.instruction
			};
			// if instruction's source starts on a new line add the source to instruction
			if (instruction.line !== undefined && lastLine !== instruction.line) {
				lastLine = instruction.line;
				instr.location = loc;
				instr.line = this.convertDebuggerLineToClient(instruction.line);
			}
			return instr;
		});

		response.body = {
			instructions: instructions
		};
		this.sendResponse(response);
	}

	protected setInstructionBreakpointsRequest(response: DebugProtocol.SetInstructionBreakpointsResponse, args: DebugProtocol.SetInstructionBreakpointsArguments) {
		// clear all instruction breakpoints
		this._runtime.clearInstructionBreakpoints();

		// set instruction breakpoints
		const breakpoints = args.breakpoints.map(ibp => {
			const address = parseInt(ibp.instructionReference);
			const offset = ibp.offset || 0;
			return <DebugProtocol.Breakpoint>{
				verified: this._runtime.setInstructionBreakpoint(address + offset)
			};
		});

		response.body = {
			breakpoints: breakpoints
		};
		this.sendResponse(response);
	}

	protected customRequest(command: string, response: DebugProtocol.Response, args: any) {
		if (command === 'toggleFormatting') {
			this._valuesInHex = ! this._valuesInHex;
			if (this._useInvalidatedEvent) {
				this.sendEvent(new InvalidatedEvent( ['variables'] ));
			}
			this.sendResponse(response);
		} else {
			super.customRequest(command, response, args);
		}
	}

	//---- helpers

	private convertToRuntime(value: string): IRuntimeVariableType {

		value= value.trim();

		if (value === 'true') {
			return true;
		}
		if (value === 'false') {
			return false;
		}
		if (value[0] === '\'' || value[0] === '"') {
			return value.substr(1, value.length-2);
		}
		const n = parseFloat(value);
		if (!isNaN(n)) {
			return n;
		}
		return value;
	}
	
	private convertFromRuntime(v: RuntimeVariable): DebugProtocol.Variable {

		let dapVariable: DebugProtocol.Variable = {
			name: v.Name,
			value: v.Val,
			type: v.Type,
			variablesReference: 0,
			evaluateName: '$' + v.Name
		};
		switch (v.Type) {
			case 'None':
				dapVariable.value = "None";
				dapVariable.type = 'None';
				break;
			case 'Int64':
				dapVariable.value = v.Val.toString();
				dapVariable.type = 'integer';
				break;
			case 'Double':
				dapVariable.value = v.Val.toString();
				dapVariable.type = 'float';
				break;
			case 'Str':
				dapVariable.value = `"${v.Val}"`;
				break;
			case 'boolean':
				dapVariable.value = v.Val ? 'true' : 'false';
				break;
			case 'Function':
				dapVariable.type = 'Function';
				dapVariable.value = `ƒ ${v.Val}`;
				break;
			case 'Event':
				dapVariable.type = 'Event';
				dapVariable.value = `e ${v.Val}`;
				break;				
			case 'Package':
				v.reference = this._runtime.createScopeRef(
					v.Type, v.FrameId, v.Val,v.Id);
				dapVariable.value = 'Package(Size:' + v.Size.toString() + ")";
				dapVariable.type = "Package";
				dapVariable.variablesReference = v.reference;
				dapVariable.namedVariables = v.Size;
				break;
			case 'RemoteObject':
				v.reference = this._runtime.createScopeRef(
					v.Type, v.FrameId, v.Val,v.Id);
				dapVariable.value = 'RemoteObject(Size:' + v.Size.toString() + ")";
				dapVariable.type = "RemoteObject";
				dapVariable.variablesReference = v.reference;
				dapVariable.namedVariables = v.Size;
				break;							
			case 'Class':
				v.reference = this._runtime.createScopeRef(
					v.Type, v.FrameId, v.Val,v.Id);
				dapVariable.value = 'Class(Size:' + v.Size.toString() + ")";
				dapVariable.type = "Class";
				dapVariable.variablesReference = v.reference;
				dapVariable.namedVariables = v.Size;
				break;
			case 'Dict':
				v.reference = this._runtime.createScopeRef(
					v.Type, v.FrameId, v.Val,v.Id);
				dapVariable.value = 'Dict(Size:' + v.Size.toString() + ")";
				dapVariable.type = "Dict";
				dapVariable.variablesReference = v.reference;
				dapVariable.namedVariables = v.Size;
				break;
			case 'Scope.Special':
				v.reference = this._runtime.createScopeRef(
					v.Type,v.FrameId,v.Val,v.Id);
				dapVariable.value = "("+v.Size.toString()+")";
				dapVariable.type = "Scope.Special";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;				
			case 'List':
				v.reference = this._runtime.createScopeRef(
					v.Type,v.FrameId,v.Val,v.Id);
				dapVariable.value = 'List(Size:'+v.Size.toString()+")";
				dapVariable.type = "List";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;
			case 'Tensor':
				v.reference = this._runtime.createScopeRef(
					v.Type,v.FrameId,v.Val,v.Id);
				dapVariable.value = 'Tensor(Size:'+v.Size.toString()+")";
				dapVariable.type = "Tensor";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;		
			case 'Prop':
				v.reference = this._runtime.createScopeRef(
					v.Type,v.FrameId,v.Val,v.Id);
				dapVariable.value = 'Prop(Size:'+v.Size.toString()+")";
				dapVariable.type = "Prop";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;
			case 'TableRow':
				v.reference = this._runtime.createScopeRef(
					v.Type, v.FrameId, v.Val,v.Id);
				dapVariable.value = 'TableRow(ColNum:' + v.Size.toString() + ")";
				dapVariable.type = "TableRow";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;
			case 'Table':
				v.reference = this._runtime.createScopeRef(
					v.Type, v.FrameId, v.Val,v.Id);
				dapVariable.value = 'Table(RowNum:' + v.Size.toString() + ")";
				dapVariable.type = "Table";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;
			case 'PyObject':
				v.reference = this._runtime.createScopeRef(
					v.Type,v.FrameId,v.Val,v.Id);
				dapVariable.value = 'Python Object(Size:'+v.Size.toString()+")";
				dapVariable.type = "PyObject";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;	
			case 'DeferredObject':
				v.reference = this._runtime.createScopeRef(
					v.Type,v.FrameId,v.Val,v.Id);
				dapVariable.value = 'Deferred Object(Size:'+v.Size.toString()+")";
				dapVariable.type = "DeferredObject";
				dapVariable.variablesReference = v.reference;
				dapVariable.indexedVariables = v.Size;
				break;						
			default:
				break;
		}

		return dapVariable;
	}

	private formatAddress(x: number, pad = 8) {
		return this._addressesInHex ? '0x' + x.toString(16).padStart(8, '0') : x.toString(10);
	}

	private formatNumber(x: number) {
		return this._valuesInHex ? '0x' + x.toString(16) : x.toString(10);
	}

	private createSource(filePath: string): Source {
		return new Source(basename(filePath), this.convertDebuggerPathToClient(filePath), undefined, undefined, 'xLang-adapter-data');
	}

	private async getValidPort() : Promise<number>
	{
		let port : number = 35000;
		for(let i = 0; i < 1000; ++i)
		{
			const ret = await this.checkPort(port);
			if (ret > 0) {
				return ret;
			}
			else{
				port += 1;
			}
		}
		return 0;
	}

	private async checkPort(port) : Promise<number> {
        return new Promise((resolve, reject) => {
            let server = net.createServer().listen(port);
            server.on('listening', function () {
                server.close();
                resolve(port);
            });
            server.on('error', function (err) {
				server.close();
                resolve(0);
            });
        });
	}
}

