"use strict";
/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
Object.defineProperty(exports, "__esModule", { value: true });
exports.XLangDebugSession = void 0;
const debugadapter_1 = require("@vscode/debugadapter");
const path_browserify_1 = require("path-browserify");
const xLangRuntime_1 = require("./xLangRuntime");
const await_notify_1 = require("await-notify");
const base64 = require("base64-js");
const vscode = require("vscode");
//import * as path from 'path';
const net = require("net");
//import * as cp from 'child_process';
const fs = require("fs");
const os = require("os");
const crypto = require("crypto");
function isPathMatchOs(path) {
    const platform = os.platform();
    if (platform === 'win32') {
        return /^[a-zA-Z]:[\\\/].*/.test(path);
    }
    else if (platform === 'linux' || platform === 'darwin') {
        return /^[\/].*/.test(path);
    }
    return false;
}
function calMD5(str) {
    const hash = crypto.createHash('md5');
    hash.update(str);
    return hash.digest('hex');
}
function calFileMD5(filePath) {
    let content = fs.readFileSync(filePath, 'utf-8');
    content = content.replace(/\r\n/g, '\n');
    return calMD5(content);
}
class XLangDebugSession extends debugadapter_1.LoggingDebugSession {
    /**
     * Creates a new debug adapter that is used for one debug session.
     * We configure the default implementation of a debug adapter here.
     */
    constructor() {
        super("xLang.txt");
        this._variableHandles = new debugadapter_1.Handles();
        this._configurationDone = new await_notify_1.Subject();
        this._xlangStarted = new await_notify_1.Subject();
        this._cancellationTokens = new Map();
        this._reportProgress = false;
        this._progressId = 10000;
        this._cancelledProgressId = undefined;
        this._isProgressCancellable = true;
        this._valuesInHex = false;
        this._useInvalidatedEvent = false;
        this._addressesInHex = true;
        this._isLaunch = true;
        this._mapFrameIdThreadId = new Map();
        this._srcMd5List = [];
        this._srcEntryPath = "";
        this._mapSrcMd5 = new Map();
        this._mapMd5Src = new Map();
        //private _xlangProcess;
        this._serverEnded = false;
        // this debugger uses zero-based lines and columns
        this.setDebuggerLinesStartAt1(false);
        this.setDebuggerColumnsStartAt1(false);
        this._runtime = new xLangRuntime_1.XLangRuntime();
        // setup event handlers
        this._runtime.on('threadStarted', (threadID) => {
            this.sendEvent(new debugadapter_1.ThreadEvent('started', threadID));
        });
        this._runtime.on('threadExited', (threadID) => {
            this.sendEvent(new debugadapter_1.ThreadEvent('exited', threadID));
        });
        this._runtime.on('stopOnEntry', (threadID) => {
            this.sendEvent(new debugadapter_1.StoppedEvent('entry', threadID));
        });
        this._runtime.on('stopOnStep', (threadID) => {
            this.sendEvent(new debugadapter_1.StoppedEvent('step', threadID));
        });
        this._runtime.on('stopOnBreakpoint', (threadID) => {
            this.sendEvent(new debugadapter_1.StoppedEvent('breakpoint', threadID));
        });
        this._runtime.on('stopOnDataBreakpoint', (threadID) => {
            this.sendEvent(new debugadapter_1.StoppedEvent('data breakpoint', threadID));
        });
        this._runtime.on('stopOnInstructionBreakpoint', (threadID) => {
            this.sendEvent(new debugadapter_1.StoppedEvent('instruction breakpoint', threadID));
        });
        this._runtime.on('stopOnException', (exception, threadID) => {
            if (exception) {
                this.sendEvent(new debugadapter_1.StoppedEvent(`exception(${exception})`, threadID));
            }
            else {
                this.sendEvent(new debugadapter_1.StoppedEvent('exception', threadID));
            }
        });
        this._runtime.on('breakpointValidated', (bp) => {
            this.sendEvent(new debugadapter_1.BreakpointEvent('changed', { verified: bp.verified, id: bp.id }));
        });
        this._runtime.on('output', (type, text, filePath, line, column, threadID) => {
            let category;
            switch (type) {
                case 'prio':
                    category = 'important';
                    break;
                case 'out':
                    category = 'stdout';
                    break;
                case 'err':
                    category = 'stderr';
                    break;
                default:
                    category = 'console';
                    break;
            }
            const e = new debugadapter_1.OutputEvent(`${text}\n`, category);
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
            this._serverEnded = true;
            this.sendEvent(new debugadapter_1.TerminatedEvent());
        });
        this._runtime.on('breakpointState', (md5, line, actualLine) => {
            if (actualLine === -1) { // failed
                this.sendEvent(new debugadapter_1.BreakpointEvent('changed', { verified: false, id: this.getBreakpointIdMd5(md5, line, 0) }));
            }
            else {
                this.sendEvent(new debugadapter_1.BreakpointEvent('changed', { verified: true, id: this.getBreakpointIdMd5(md5, line, 0), line: actualLine }));
            }
        });
        this._runtime.on('moduleLoaded', (path, md5) => {
            this._runtime.addOutput(`module(file has breakpoints) loaded: "${md5}"   "${path}"`);
        });
        this._runtime.on('xlangStarted', (started) => {
            this._xlangStarted.notifyValue = started;
            this._xlangStarted.notify();
        });
    }
    getRuntime() {
        return this._runtime;
    }
    // use src md5 index， origin line and column to make a unique id 
    getBreakpointIdMd5(md5, line, column) {
        let srcIdx = this._srcMd5List.indexOf(md5);
        return srcIdx * 10000000 + line * 1000 + column;
    }
    /**
     * The 'initialize' request is the first request called by the frontend
     * to interrogate the features the debug adapter provides.
     */
    initializeRequest(response, args) {
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
        response.body.completionTriggerCharacters = [".", "["];
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
        response.body.supportsEvaluateForHovers = true;
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
    configurationDoneRequest(response, args) {
        super.configurationDoneRequest(response, args);
        // notify the launchRequest that configuration has finished
        this._configurationDone.notify();
    }
    disconnectRequest(response, args, request) {
        console.log(`disconnectRequest suspend: ${args.suspendDebuggee}, terminate: ${args.terminateDebuggee}`);
        if (this._terminal) {
            this._terminal.dispose();
            this._terminal = undefined;
        }
        if (this._serverEnded)
            this._runtime.close(true);
        else {
            if (this._isLaunch && args.terminateDebuggee) {
                this._runtime.close(true);
            }
            else {
                this._runtime.close(false);
            }
        }
        this.sendResponse(response);
    }
    restartRequest(response, args, request) {
        this.sendResponse(response);
    }
    async attachRequest(response, args) {
        this._runtime.addOutput("--------------------------------------------------");
        this._isLaunch = false;
        this._runtime.runMode = 'attach';
        this._runtime.serverAddress = args.dbgIp || "localhost";
        this._runtime.serverPort = Number(args.dbgPort || 3142);
        this.startDebug(response, args);
    }
    async launchRequest(response, args) {
        this._runtime.addOutput("--------------------------------------------------");
        let port = await this.getValidPort();
        let xlangBin = vscode.workspace.getConfiguration('XLangDebugger').get('ExePath');
        // launch xlang in vscode's terminal
        this._terminal = vscode.window.createTerminal(`XLang server ${port}`);
        this._terminal.show();
        this._terminal.sendText(`${xlangBin} -event_loop -dbg -enable_python -port ${port}`);
        this._runtime.serverAddress = "localhost";
        this._runtime.serverPort = port;
        this._isLaunch = true;
        this._runtime.runMode = 'launch';
        this.startDebug(response, args);
    }
    async startDebug(response, args) {
        this._mapSrcMd5.clear();
        this._mapMd5Src.clear();
        this._srcEntryPath = this._runtime.normalizePathAndCasing(args.program);
        let md5 = calFileMD5(this._srcEntryPath);
        this._runtime.checkStarted(this._srcEntryPath, md5);
        await this._xlangStarted.wait();
        if (!this._xlangStarted.notifyValue) {
            await vscode.window.showErrorMessage(`can not connect to a xlang dbg server at ${this._runtime.serverAddress}:${this._runtime.serverPort} debugging stopped`, { modal: true }, "OK");
            this.sendResponse(response);
            this.sendEvent(new debugadapter_1.TerminatedEvent());
            return;
        }
        this._serverEnded = false;
        // make sure to 'Stop' the buffered logging if 'trace' is not set
        debugadapter_1.logger.setup(args.trace ? debugadapter_1.Logger.LogLevel.Verbose : debugadapter_1.Logger.LogLevel.Stop, false);
        this._mapSrcMd5.set(this._srcEntryPath, md5);
        this._mapMd5Src.set(md5, this._srcEntryPath);
        this._runtime.addOutput(`load entry source file on server: "${md5}"   "${this._srcEntryPath}"`);
        let retVal = await this._runtime.loadSource(this._srcEntryPath);
        if (retVal == -1) // is run file
         {
            this.sendResponse(response);
            //this.sendEvent(new TerminatedEvent());
            vscode.window.showInformationMessage(`file "${this._runtime.runFile}" is running`, { modal: true }, "OK");
            return;
        }
        this.sendEvent(new debugadapter_1.InitializedEvent());
        // wait configuration has finished (and configurationDoneRequest has been called)
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
        }
        else {
            this.sendResponse(response);
        }
    }
    setFunctionBreakPointsRequest(response, args, request) {
        this.sendResponse(response);
    }
    async setBreakPointsRequest(response, args) {
        const path = this._runtime.normalizePathAndCasing(args.source.path);
        //const clientLines = args.lines || [];
        const clientLines = args.breakpoints?.map(col => { return col.line; }) || [];
        let bNew = false;
        let md5 = "";
        if (!this._mapSrcMd5.has(path)) {
            md5 = calFileMD5(path);
            console.log(path, "  ", md5);
            this._mapSrcMd5.set(path, md5);
            this._mapMd5Src.set(md5, path);
            bNew = true; //this._runtime.addOutput(`breakpoint source file: "${md5}"   "${path}"`);
        }
        else {
            md5 = this._mapSrcMd5.get(path) || "";
        }
        let srcIdx = this._srcMd5List.indexOf(md5);
        if (srcIdx < 0) {
            this._srcMd5List.push(md5);
            srcIdx = this._srcMd5List.length - 1;
        }
        this._runtime.setBreakPoints(path, md5, clientLines, (lines) => {
            let actualBreakpoints = [];
            let bModuleLoaded = true;
            for (let i = 0; i < lines.length; i += 2) {
                let bp;
                let l = lines[i]; // origin line
                let al = lines[i + 1]; // actual line
                let id = this.getBreakpointIdMd5(md5, l, 0); // breakpoint id
                if (al === -2) { //'pending'
                    bModuleLoaded = false;
                    bp = new debugadapter_1.Breakpoint(false, l);
                    bp.setId(id);
                }
                else if (l === -1) { // 'failed'
                    bp = new debugadapter_1.Breakpoint(false, l);
                    bp.setId(id);
                }
                else { // 'valid'
                    bp = new debugadapter_1.Breakpoint(true, al);
                    bp.setId(id);
                }
                actualBreakpoints.push(bp);
            }
            if (bNew) {
                if (bModuleLoaded === false)
                    this._runtime.addOutput(`source file has breakpoints: "${md5}"   "${path}" is not loaded on server`);
                else
                    this._runtime.addOutput(`source file has breakpoints: "${md5}"   "${path}" has loaded on server`);
            }
            response.body = {
                breakpoints: actualBreakpoints
            };
            this.sendResponse(response);
        });
    }
    breakpointLocationsRequest(response, args, request) {
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
        }
        else {
            response.body = {
                breakpoints: []
            };
        }
        this.sendResponse(response);
    }
    async setExceptionBreakPointsRequest(response, args) {
        let namedException = undefined;
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
    exceptionInfoRequest(response, args) {
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
    threadsRequest(response) {
        this._runtime.getThreads((threads) => {
            let tids = [];
            response.body = {
                threads: threads.map((t) => {
                    tids.push(t.id);
                    const thread = new debugadapter_1.Thread(t.id, t.name);
                    return thread;
                }),
            };
            this._mapFrameIdThreadId.forEach((value, key) => {
                if (!tids.includes(value)) {
                    this._mapFrameIdThreadId.delete(key);
                }
            });
            this.sendResponse(response);
        });
    }
    stackTraceRequest(response, args) {
        const threadId = args.threadId;
        const startFrame = typeof args.startFrame === 'number' ? args.startFrame : 0;
        const maxLevels = typeof args.levels === 'number' ? args.levels : 1000;
        const endFrame = startFrame + maxLevels;
        this._mapFrameIdThreadId.forEach((value, key) => {
            if (value === threadId) {
                this._mapFrameIdThreadId.delete(key);
            }
        });
        this._runtime.stack(threadId, startFrame, endFrame, (stk) => {
            response.body = {
                stackFrames: stk.frames.map((f, ix) => {
                    this._mapFrameIdThreadId.set(f.id, threadId);
                    const sf = new debugadapter_1.StackFrame(f.id, f.name, this.createSourceMd5(f.md5, f.file), this.convertDebuggerLineToClient(f.line));
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
                totalFrames: stk.count // stk.count is the correct size, should result in a max. of two requests
                //totalFrames: 1000000 			// not the correct size, should result in a max. of two requests
                //totalFrames: endFrame + 20 	// dynamically increases the size with every requested chunk, results in paging
            };
            this.sendResponse(response);
        });
    }
    scopesRequest(response, args) {
        response.body = {
            scopes: [
                new debugadapter_1.Scope("Locals", this._runtime.createScopeRef('locals', args.frameId, null, null), false),
                new debugadapter_1.Scope("Globals", this._runtime.createScopeRef('globals', args.frameId, null, null), true)
            ]
        };
        this.sendResponse(response);
    }
    async writeMemoryRequest(response, { data, memoryReference, offset = 0 }) {
        const variable = this._variableHandles.get(Number(memoryReference));
        if (typeof variable === 'object') {
            const decoded = base64.toByteArray(data);
            //TODO:variable.setMemory(decoded, offset);
            response.body = { bytesWritten: decoded.length };
        }
        else {
            response.body = { bytesWritten: 0 };
        }
        this.sendResponse(response);
        this.sendEvent(new debugadapter_1.InvalidatedEvent(['variables']));
    }
    async readMemoryRequest(response, { offset = 0, count, memoryReference }) {
        const variable = this._variableHandles.get(Number(memoryReference));
        if (typeof variable === 'object' && variable.memory) {
            const memory = variable.memory.subarray(Math.min(offset, variable.memory.length), Math.min(offset + count, variable.memory.length));
            response.body = {
                address: offset.toString(),
                data: base64.fromByteArray(memory),
                unreadableBytes: count - memory.length
            };
        }
        else {
            response.body = {
                address: offset.toString(),
                data: '',
                unreadableBytes: count
            };
        }
        this.sendResponse(response);
    }
    async variablesRequest(response, args, request) {
        let cb = (vs) => {
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
            this._runtime.getLocalVariables(threadId, frameId, cb);
        }
        else if (varType === 'globals') {
            this._runtime.getGlobalVariables(threadId, cb);
        }
        else {
            this._runtime.getObject(threadId, frameId, varType, objId, args.start === undefined ? 0 : args.start, args.count === undefined ? -1 : args.count, cb);
        }
    }
    setVariableRequest(response, args) {
        let cb = (v) => {
            response.body = this.convertFromRuntime(v);
            this.sendResponse(response);
        };
        const varInfo = this._runtime.getScopeRef(args.variablesReference);
        const varType = varInfo[0];
        const frameId = varInfo[1];
        const objId = varInfo[3];
        const threadId = this._mapFrameIdThreadId.get(frameId);
        this._runtime.setObject(threadId, frameId, varType, objId, args.name, args.value, cb);
    }
    continueRequest(response, args) {
        this._runtime.continue(false, args.threadId, () => {
            this.sendResponse(response);
        });
    }
    reverseContinueRequest(response, args) {
        this._runtime.continue(false, args.threadId, () => {
            this.sendResponse(response);
        });
    }
    nextRequest(response, args) {
        this._runtime.step(args.granularity === 'instruction', false, args.threadId, () => {
            this.sendResponse(response);
        });
    }
    stepBackRequest(response, args) {
        this._runtime.step(args.granularity === 'instruction', true, args.threadId, () => {
            this.sendResponse(response);
        });
    }
    stepInTargetsRequest(response, args) {
        const targets = this._runtime.getStepInTargets(args.frameId);
        response.body = {
            targets: targets.map(t => {
                return { id: t.id, label: t.label };
            })
        };
        this.sendResponse(response);
    }
    stepInRequest(response, args) {
        this._runtime.stepIn(args.threadId, args.targetId, () => {
            this.sendResponse(response);
        });
    }
    stepOutRequest(response, args) {
        this._runtime.stepOut(args.threadId, () => {
            this.sendResponse(response);
        });
    }
    async evaluateRequest(response, args) {
        let reply;
        let rv;
        switch (args.context) {
            case 'repl':
                // handle some REPL commands:
                // 'evaluate' supports to create and delete breakpoints from the 'repl':
                const matches = /new +([0-9]+)/.exec(args.expression);
                if (matches && matches.length === 2) {
                    const mbp = await this._runtime.setBreakPoint(this._runtime.sourceFile, this.convertClientLineToDebugger(parseInt(matches[1])));
                    const bp = new debugadapter_1.Breakpoint(mbp.verified, this.convertDebuggerLineToClient(mbp.line), undefined, this.createSource(this._runtime.sourceFile));
                    bp.id = mbp.id;
                    this.sendEvent(new debugadapter_1.BreakpointEvent('new', bp));
                    reply = `breakpoint created`;
                }
                else {
                    const matches = /del +([0-9]+)/.exec(args.expression);
                    if (matches && matches.length === 2) {
                        const mbp = this._runtime.clearBreakPoint(this._runtime.sourceFile, this.convertClientLineToDebugger(parseInt(matches[1])));
                        if (mbp) {
                            const bp = new debugadapter_1.Breakpoint(false);
                            bp.id = mbp.id;
                            this.sendEvent(new debugadapter_1.BreakpointEvent('removed', bp));
                            reply = `breakpoint deleted`;
                        }
                    }
                    else {
                        const matches = /progress/.exec(args.expression);
                        if (matches && matches.length === 1) {
                            if (this._reportProgress) {
                                reply = `progress started`;
                                this.progressSequence();
                            }
                            else {
                                reply = `frontend doesn't support progress (capability 'supportsProgressReporting' not set)`;
                            }
                        }
                    }
                }
            // fall through
            default:
                if (args.expression.startsWith('$')) {
                    rv = this._runtime.getLocalVariable(args.expression.substr(1));
                    break;
                }
                else {
                    const frameId = args.frameId || 0;
                    this._runtime.evaluate(args.expression, frameId, (v) => {
                        if (v) {
                            const dapVar = this.convertFromRuntime(v);
                            response.body = {
                                result: dapVar.value,
                                type: dapVar.type,
                                variablesReference: dapVar.variablesReference,
                                presentationHint: dapVar.presentationHint
                            };
                            this.sendResponse(response);
                        }
                        else {
                            // fallback: check if it is a local variable
                            const threadId = this._mapFrameIdThreadId.get(frameId);
                            if (threadId !== undefined) {
                                this._runtime.getLocalVariables(threadId, frameId, (vars) => {
                                    const match = vars ? vars.find(x => x.Name === args.expression) : undefined;
                                    if (match) {
                                        const dapVar = this.convertFromRuntime(match);
                                        response.body = {
                                            result: dapVar.value,
                                            type: dapVar.type,
                                            variablesReference: dapVar.variablesReference,
                                            presentationHint: dapVar.presentationHint
                                        };
                                    }
                                    else {
                                        response.body = {
                                            result: reply ? reply : `not available`,
                                            variablesReference: 0
                                        };
                                    }
                                    this.sendResponse(response);
                                });
                            }
                            else {
                                response.body = {
                                    result: reply ? reply : `not available`,
                                    variablesReference: 0
                                };
                                this.sendResponse(response);
                            }
                        }
                    });
                    return;
                }
        }
        if (rv) {
            const v = this.convertFromRuntime(rv);
            response.body = {
                result: v.value,
                type: v.type,
                variablesReference: v.variablesReference,
                presentationHint: v.presentationHint
            };
        }
        else {
            response.body = {
                result: reply ? reply : `not available`,
                variablesReference: 0
            };
        }
        this.sendResponse(response);
    }
    setExpressionRequest(response, args) {
        if (args.expression.startsWith('$')) {
            const rv = this._runtime.getLocalVariable(args.expression.substr(1));
            if (rv) {
                rv.Val = this.convertToRuntime(args.value);
                response.body = this.convertFromRuntime(rv);
                this.sendResponse(response);
            }
            else {
                this.sendErrorResponse(response, {
                    id: 1002,
                    format: `variable '{lexpr}' not found`,
                    variables: { lexpr: args.expression },
                    showUser: true
                });
            }
        }
        else {
            this.sendErrorResponse(response, {
                id: 1003,
                format: `'{lexpr}' not an assignable expression`,
                variables: { lexpr: args.expression },
                showUser: true
            });
        }
    }
    async progressSequence() {
        const ID = '' + this._progressId++;
        await (0, xLangRuntime_1.timeout)(100);
        const title = this._isProgressCancellable ? 'Cancellable operation' : 'Long running operation';
        const startEvent = new debugadapter_1.ProgressStartEvent(ID, title);
        startEvent.body.cancellable = this._isProgressCancellable;
        this._isProgressCancellable = !this._isProgressCancellable;
        this.sendEvent(startEvent);
        this.sendEvent(new debugadapter_1.OutputEvent(`start progress: ${ID}\n`));
        let endMessage = 'progress ended';
        for (let i = 0; i < 100; i++) {
            await (0, xLangRuntime_1.timeout)(500);
            this.sendEvent(new debugadapter_1.ProgressUpdateEvent(ID, `progress: ${i}`));
            if (this._cancelledProgressId === ID) {
                endMessage = 'progress cancelled';
                this._cancelledProgressId = undefined;
                this.sendEvent(new debugadapter_1.OutputEvent(`cancel progress: ${ID}\n`));
                break;
            }
        }
        this.sendEvent(new debugadapter_1.ProgressEndEvent(ID, endMessage));
        this.sendEvent(new debugadapter_1.OutputEvent(`end progress: ${ID}\n`));
        this._cancelledProgressId = undefined;
    }
    dataBreakpointInfoRequest(response, args) {
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
                response.body.accessTypes = ["write"];
                response.body.canPersist = true;
            }
            else {
                response.body.dataId = args.name;
                response.body.description = args.name;
                response.body.accessTypes = ["read", "write", "readWrite"];
                response.body.canPersist = true;
            }
        }
        this.sendResponse(response);
    }
    setDataBreakpointsRequest(response, args) {
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
    completionsRequest(response, args) {
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
    cancelRequest(response, args) {
        if (args.requestId) {
            this._cancellationTokens.set(args.requestId, true);
        }
        if (args.progressId) {
            this._cancelledProgressId = args.progressId;
        }
    }
    disassembleRequest(response, args) {
        const baseAddress = parseInt(args.memoryReference);
        const offset = args.instructionOffset || 0;
        const count = args.instructionCount;
        const isHex = args.memoryReference.startsWith('0x');
        const pad = isHex ? args.memoryReference.length - 2 : args.memoryReference.length;
        const loc = this.createSource(this._runtime.sourceFile);
        let lastLine = -1;
        const instructions = this._runtime.disassemble(baseAddress + offset, count).map(instruction => {
            const address = instruction.address.toString(isHex ? 16 : 10).padStart(pad, '0');
            const instr = {
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
    setInstructionBreakpointsRequest(response, args) {
        // clear all instruction breakpoints
        this._runtime.clearInstructionBreakpoints();
        // set instruction breakpoints
        const breakpoints = args.breakpoints.map(ibp => {
            const address = parseInt(ibp.instructionReference);
            const offset = ibp.offset || 0;
            return {
                verified: this._runtime.setInstructionBreakpoint(address + offset)
            };
        });
        response.body = {
            breakpoints: breakpoints
        };
        this.sendResponse(response);
    }
    customRequest(command, response, args) {
        if (command === 'toggleFormatting') {
            this._valuesInHex = !this._valuesInHex;
            if (this._useInvalidatedEvent) {
                this.sendEvent(new debugadapter_1.InvalidatedEvent(['variables']));
            }
            this.sendResponse(response);
        }
        else {
            super.customRequest(command, response, args);
        }
    }
    //---- helpers
    convertToRuntime(value) {
        value = value.trim();
        if (value === 'true') {
            return true;
        }
        if (value === 'false') {
            return false;
        }
        if (value[0] === '\'' || value[0] === '"') {
            return value.substr(1, value.length - 2);
        }
        const n = parseFloat(value);
        if (!isNaN(n)) {
            return n;
        }
        return value;
    }
    convertFromRuntime(v) {
        let dapVariable = {
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
            case 'byte':
                dapVariable.value = "0x" + v.Val.toString(16).padStart(2, '0');
                if (v.Val >= 32 && v.Val <= 126) {
                    dapVariable.value += "'" + String.fromCharCode(v.Val) + "'";
                }
                dapVariable.type = 'byte';
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
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Package(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Package";
                dapVariable.variablesReference = v.reference;
                dapVariable.namedVariables = v.Size;
                break;
            case 'RemoteObject':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'RemoteObject(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "RemoteObject";
                dapVariable.variablesReference = v.reference;
                dapVariable.namedVariables = v.Size;
                break;
            case 'Class':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Class(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Class";
                dapVariable.variablesReference = v.reference;
                dapVariable.namedVariables = v.Size;
                break;
            case 'Dict':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Dict(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Dict";
                dapVariable.variablesReference = v.reference;
                dapVariable.namedVariables = v.Size;
                break;
            case 'Scope.Special':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = "(" + v.Size.toString() + ")";
                dapVariable.type = "Scope.Special";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'List':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'List(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "List";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'Set':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Set(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Set";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = undefined;
                dapVariable.namedVariables = v.Size;
                break;
            case 'Tensor':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Tensor(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Tensor";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'Prop':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Prop(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Prop";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'TableRow':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'TableRow(ColNum:' + v.Size?.toString() + ")";
                dapVariable.type = "TableRow";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'Table':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Table(RowNum:' + v.Size?.toString() + ")";
                dapVariable.type = "Table";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'PyObject':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Python Object(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "PyObject";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'DeferredObject':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Deferred Object(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "DeferredObject";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            case 'Binary':
                v.reference = this._runtime.createScopeRef(v.Type, v.FrameId, v.Val, v.Id);
                dapVariable.value = 'Binary(Size:' + v.Size?.toString() + ")";
                dapVariable.type = "Binary";
                dapVariable.variablesReference = v.reference;
                dapVariable.indexedVariables = v.Size;
                break;
            default:
                break;
        }
        return dapVariable;
    }
    formatAddress(x, pad = 8) {
        return this._addressesInHex ? '0x' + x.toString(16).padStart(8, '0') : x.toString(10);
    }
    createSource(filePath) {
        return new debugadapter_1.Source((0, path_browserify_1.basename)(filePath), this.convertDebuggerPathToClient(filePath), undefined, undefined, 'xLang-adapter-data');
    }
    createSourceMd5(md5, src_path) {
        let path = "";
        if (this._mapMd5Src.has(md5))
            path = this._mapMd5Src.get(md5) || "";
        if (path === "") {
            if (isPathMatchOs(src_path)) {
                if (this._mapSrcMd5.has(src_path)) {
                    let local_md5 = this._mapSrcMd5.get(src_path);
                    this._runtime.addOutput(`source file "${src_path}" on server md5 not match, local: "${local_md5}", server: "${md5}"`);
                }
                else {
                    let exists = fs.existsSync(src_path);
                    if (exists) {
                        let local_md5 = calFileMD5(src_path);
                        if (local_md5 === md5)
                            path = src_path;
                        else
                            this._runtime.addOutput(`source file "${src_path}" on server md5 not match, local: "${local_md5}", server: "${md5}"`);
                    }
                    else
                        this._runtime.addOutput(`source file "${src_path}" on server not exists on local`);
                }
            }
            else
                this._runtime.addOutput(`Current os is not same with server, please open source file accroding to "${src_path}" and add breakpoints first`);
        }
        return new debugadapter_1.Source((0, path_browserify_1.basename)(path), this.convertDebuggerPathToClient(path), undefined, undefined, 'xLang-adapter-data');
    }
    async getValidPort() {
        let port = 35000;
        for (let i = 0; i < 1000; ++i) {
            const ret = await this.checkPort(port);
            if (ret > 0) {
                return ret;
            }
            else {
                port += 1;
            }
        }
        return 0;
    }
    async checkPort(port) {
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
exports.XLangDebugSession = XLangDebugSession;
//# sourceMappingURL=xLangDebug.js.map