/*---------------------------------------------------------
 * https://microsoft.github.io/debug-adapter-protocol/overview
 *--------------------------------------------------------*/

import { EventEmitter } from 'events';
import * as vscode from 'vscode';
import * as os from 'os';
import * as fs from 'fs';

export interface IRuntimeBreakpoint {
	id: number;
	line: number;
	verified: boolean;
}

interface IRuntimeStepInTargets {
	id: number;
	label: string;
}

interface IRuntimeStackFrame {
	id: number;
	name: string;
	file: string;
	line: number;
	column?: number;
	instruction?: number;
}

interface RuntimeDisassembledInstruction {
	address: number;
	instruction: string;
	line?: number;
}
export class RuntimeVariable {
	private _name: string = "";
	private _value: any = null;
	private _id:string = "";
	private _size: number = 0;
	private _type: string = "";
	private _frameId: number = 0;
	private _reference: number = 0;
	constructor(name: string, value, id, type: string,size:number,frmId:number) {
		this._name = name;
		this._type = type;
		this._value = value;
		this._id = id;
		this._size = size;
		this._frameId = frmId;
	}
	public get FrameId() {
		return this._frameId;
	}
	public get Name() {
		return this._name;
	}
	public set Name(nm) {
		this._name =nm;
	}
	public get Size() { return this._size; }
	public get reference() {
		return this._reference;
	}
	public set reference(r) {
		this._reference = r;
	}
	public get Type() {
		return this._type;
	}
	public set Type(t) {
		this._type = t;
	}
	public get Val() {
		return this._value;
	}
	public set Val(v) {
		this._value = v;
	}
	public get Id() {
		return this._id;
	}
	public set Id(v) {
		this._id = v;
	}
}

export function timeout(ms: number) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

export class XLangRuntime extends EventEmitter {

	private _sourceFile: string = '';
	private _moduleKey: number = 0;
	private _sessionRunning: boolean = false;
	private _srvaddress:string ="localhost";
	private _srvPort:number =3142;
	public get sourceFile() {
		return this._sourceFile;
	}

	private sourceModuleKeyMap = new Map<string, number>();

	private instructions: Word[] = [];
	private starts: number[] = [];

	// This is the next line that will be 'executed'
	private _currentLine = 0;
	private get currentLine() {
		return this._currentLine;
	}
	public set currentLine(x) {
		this._currentLine = x;
		this.instruction = this.starts[x];
	}

	public get serverPort(){
		return this._srvPort;
	}

	public set serverPort(port){
		this._srvPort = port;
	}

	public get serverAddress(){
		return this._srvaddress;
	}

	public set serverAddress(addr){
		this._srvaddress = addr;
	}

	private isLocalServer() : boolean
	{
		if (this._srvaddress === "127.0.0.1" || this._srvaddress.toLowerCase() === "localhost"){
			return true;
		}
		for (const iface of Object.values(os.networkInterfaces())) {
			for (const details of iface) {
				if (details.family === 'IPv4' && !details.internal) {
					if (this._srvaddress === details.address){
						return true;
					}
				}
			}
		}
		return false;
	}

	// This is the next instruction that will be 'executed'
	public instruction= 0;

	// all instruction breakpoint addresses
	private instructionBreakpoints = new Set<number>();


	private breakAddresses = new Map<string, string>();

	private namedException: string | undefined;
	private otherExceptions = false;


	constructor() {
		super();
	}
	private nextVarRef = 1;
	private varRefMap = new Map<number,[]>();
	public createScopeRef(varType, frameId,val,id) {
		let refId = this.nextVarRef++;
		this.varRefMap[refId] = [varType, frameId,val,id];
		return refId;
	}
	public getScopeRef(refId) {
		return this.varRefMap[refId];
	}

	public close() {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() + ",cmd='Terminate')";
		this.Call(code, (retData) => {});
		this.sourceModuleKeyMap.clear();
		this._sourceFile = '';
		this._moduleKey = 0;
	}
	public async loadSource(file: string): Promise<number> {
		let srcFile = this.normalizePathAndCasing(file);
		let key = this.sourceModuleKeyMap.get(srcFile);
		if (key != undefined) {
			return key;
        }
		let srcFile_x = srcFile.replaceAll('\\', '/');
		let code;
		if (this.isLocalServer())
		{
			code = "m = load('" + srcFile_x + "')\nreturn m";
		}
		else
		{
			const content = fs.readFileSync(file);
			code = "m = load('" + srcFile_x + "','" + content + "')\nreturn m";
		}
		let promise = new Promise((resolve, reject) => {
			this.Call(code, resolve);
		});
		let retVal= await promise as number;
		this.sourceModuleKeyMap.set(srcFile, retVal);
		this._sourceFile = srcFile;
		this._moduleKey = retVal;
		return retVal;
	}
	private tryTimes = 1;
	public async checkStarted()
	{
		vscode.window.showInformationMessage(`try connecting to a xlang dbg server at ${this.serverAddress}:${this.serverPort}, try ${this.tryTimes}`);
		const https = require('http');
		const options = {
			hostname: this._srvaddress,
			port: this._srvPort,
			path: '/devops/checkStarted',
			method: 'GET',
			timeout: 2000
		};
		const req = https.request(options, res => {
			this.sendEvent('xlangStarted', true);
			vscode.window.showInformationMessage(`connecting to a xlang dbg server at ${this.serverAddress}:${this.serverPort} successed`);
		});
	
		req.on('error', error => {
			if (error.errno === -4078 && this.tryTimes <= 3)
			{
				var thisObj = this;
				setTimeout(function() {
					thisObj.checkStarted();
				}, 1000);
				++this.tryTimes;
			}
			else
			{
				this.tryTimes = 1;
				this.sendEvent('xlangStarted', false);
			}
		});
		req.end();
	}

	private async fetchNotify()
	{
		const https = require('http');
		const options = {
			hostname: this._srvaddress,
			port: this._srvPort,
			path: '/devops/getnotify',
			method: 'GET'
		};
		console.log(`fetchNotify request started`);
		const req = https.request(options, res => {
			console.log(`fetchNotify->statusCode: ${res.statusCode}`);
		res.on('data', d => {
			var strData = new TextDecoder().decode(d);
			console.log(strData);
			let tagNoti = "$notify$";
			if (strData.startsWith(tagNoti)){
				var param = strData.substring(tagNoti.length);
				var notis = JSON.parse(param);
				if (notis) {
					for (let n in notis) {
						let kv = notis[n];
							if(kv.hasOwnProperty("HitBreakpoint")){
								this.sendEvent('stopOnBreakpoint', kv["threadId"]);
							}
							else if(kv.hasOwnProperty("StopOnEntry")){
								this.sendEvent('stopOnEntry', kv["StopOnEntry"]);
							}
							else if(kv.hasOwnProperty("StopOnStep")){
								this.sendEvent('stopOnStep', kv["StopOnStep"]);
							}
							else if(kv.hasOwnProperty("ThreadStarted")){
								this.sendEvent('threadStarted', kv["ThreadStarted"]);
							}
							else if(kv.hasOwnProperty("ThreadExited")){
								this.sendEvent('threadExited', kv["ThreadExited"]);
						}
							else if(kv.hasOwnProperty("BreakpointPath")){
								this.sendEvent('breakpointState', kv["BreakpointPath"], kv["line"]);
							}
					}
				}
			}
			else if(strData === "end" || strData === "error")
			{
				this._sessionRunning = false;
				this.sendEvent('end');
			}
			});

			res.on('end', ()=>{
			if(this._sessionRunning )
			{
				this.fetchNotify();
			}
		});
		});
	
		req.on('error', error => {
			console.error("fetchNotify->",error);
			if(this._sessionRunning )
			{
				var thisObj = this;
				setTimeout(function() {
					thisObj.fetchNotify();
				}, 100);
			}
		});
		req.end();
	}
	/**
	 * Start executing the loaded program.
	 */
	public async start(stopOnEntry: boolean, debug: boolean): Promise<void> {
		this._sessionRunning = true;
		this.fetchNotify();
		////this._sourceFile = this.normalizePathAndCasing(program);
		////this._moduleKey = await this.loadSource(this._sourceFile);
		if (this._moduleKey!=0) {
			let code = "tid=threadid()\nmainrun(" + this._moduleKey.toString()
				+ ", onFinish = 'fire(\"devops.dbg\",action=\"end\",tid=${tid})'"
				+ ",stopOnEntry=True)\nreturn True";
			this.Call(code, (ret) => {
				console.log(ret);
				// if (debug) {
				// 	//this.verifyBreakpoints(this._sourceFile);
				// 	////this.GetStartLine((startLine) => {
				// 		if (stopOnEntry) {
				// 			//this.currentLine = startLine - 1;
				// 			this.sendEvent('stopOnEntry', Number(ret));
				// 		} else {
				// 			// we just start to run until we hit a breakpoint, an exception, or the end of the program
				// 			this.continue(false,()=>{ });
				// 		}
				// 	////});
				// } else {
				// 	this.continue(false, () => { });
				// }
			});
		}
	}
	private Call(code,cb)
	{
		const https = require('http');
		const querystring = require('querystring');
		const parameters = {
			'code': code
		};
		const requestargs = querystring.stringify(parameters);
		const options = {
		hostname: this._srvaddress,
		port: this._srvPort,
		path: '/devops/run?'+requestargs,
		method: 'GET'
		};
		const req = https.request(options, res => {
		console.log(`statusCode: ${res.statusCode}`);
		var allData = "";
		res.on('end', () => {
			cb(allData);			
		  });
		res.on('data', d => {
			var strData = new TextDecoder().decode(d);
			allData +=strData;
		});
		});
	
		req.on('error', error => {
		console.error(error);
		});
	
		req.end();
	
	}

	public continue(reverse: boolean, threadId: number, cb) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Continue')";
		this.Call(code, (retData) => cb());
	}
	public step(instruction: boolean, reverse: boolean, threadId: number, cb:Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Step')";
		this.Call(code, (retData) => {
			//this.sendEvent('stopOnStep');
			cb();
        });
	}
	public async setBreakPoints(path: string, lines: number[], cb: Function) {
		////let mKey = await this.loadSource(this.normalizePathAndCasing(path));
		let path_x = path.replaceAll('\\', '/');
		let code = "import xdb\nreturn xdb.set_breakpoints(\"" + path_x + "\",[" + lines.join() + "])";
		this.Call(code, (retData) => {
			var retLines = JSON.parse(retData);
			cb(retLines);
		});
	}
	/**
	 * "Step into" for XLang debug means: go to next character
	 */
	public stepIn(threadId:number, targetId: number | undefined, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='StepIn')";
		this.Call(code, (retData) => {
			//this.sendEvent('stopOnStep');
			cb();
		});
	}

	/**
	 * "Step out" for XLang debug means: go to previous character
	 */
	public stepOut(threadId:number, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='StepOut')";
		this.Call(code, (retData) => {
			//this.sendEvent('stopOnStep');
			cb();
		});
	}

	public getStepInTargets(frameId: number): IRuntimeStepInTargets[] {

		const line = this.getLine();
		const words = this.getWords(this.currentLine, line);

		// return nothing if frameId is out of range
		if (frameId < 0 || frameId >= words.length) {
			return [];
		}

		const { name, index  }  = words[frameId];

		// make every character of the frame a potential "step in" target
		return name.split('').map((c, ix) => {
			return {
				id: index + ix,
				label: `target: ${c}`
			};
		});
	}

	public getThreads(cb: Function)
	{
		let code = "import xdb\nreturn xdb.get_threads()";
		this.Call(code, (retVal) => {
			var retObj = null;
			try {
				retObj = JSON.parse(retVal);
			}
			catch (err) {
				console.log("Json Parse Error:", err);
				return;
			}
			let threads = [];
			for (let i in retObj) {
				let t = retObj[i];
				threads.push({id: t["id"], name: t["name"].toString()});
			}
			cb(threads);
		});
	}

	public stack(threadId: number,startFrame: number, endFrame: number, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Stack')";
		this.Call(code, (retVal) => {
			console.log(retVal);
			var retObj = null;
			try {
				retObj = JSON.parse(retVal);
			}
			catch (err) {
				console.log("Json Parse Error:", err);
				return;
			}	
			console.log(retObj);
			const frames: IRuntimeStackFrame[] = [];
			// every word of the current line becomes a stack frame.
			for (let i in retObj) {
				let frm = retObj[i];
				let name = frm["name"];
				if (name === "") {
					name = "main";
				}
				const stackFrame: IRuntimeStackFrame = {
					id: frm["id"],
					name: name,
					file: frm["file"],//this._sourceFile,
					line: frm["line"] - 1,
					column: frm["column"]
				};
				frames.push(stackFrame);
			}

			let stk = {
				frames: frames,
				count: retObj.length
			};
			cb(stk);
        });
	}

	public setDataBreakpoint(address: string, accessType: 'read' | 'write' | 'readWrite'): boolean {

		const x = accessType === 'readWrite' ? 'read write' : accessType;

		const t = this.breakAddresses.get(address);
		if (t) {
			if (t !== x) {
				this.breakAddresses.set(address, 'read write');
			}
		} else {
			this.breakAddresses.set(address, x);
		}
		return true;
	}

	public clearAllDataBreakpoints(): void {
		this.breakAddresses.clear();
	}

	public setExceptionsFilters(namedException: string | undefined, otherExceptions: boolean): void {
		this.namedException = namedException;
		this.otherExceptions = otherExceptions;
	}

	public setInstructionBreakpoint(address: number): boolean {
		this.instructionBreakpoints.add(address);
		return true;
	}

	public clearInstructionBreakpoints(): void {
		this.instructionBreakpoints.clear();
	}

	public getGlobalVariables(threadId, cb){
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() +",cmd='Globals')";
		this.Call(code, (retVal) => {
			console.log(retVal);
			var retObj = JSON.parse(retVal);
			console.log(retObj);
			let vars = Array.from(retObj, (x: Map<string, any>) =>
				new RuntimeVariable(
					x["Name"],
					x["Value"],
					x["Id"],
					x["Type"],
					x["Size"],
					0));
			cb(vars);
        });
	}
	public getLocalVariables(threadId, frameId,cb){
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() +
			",frameId=" + frameId.toString()+",cmd='Locals')";
		this.Call(code, (retVal) => {
			console.log(retVal);
			var retObj = null;
			try {
				retObj = JSON.parse(retVal);
			}
			catch (err) {
				console.log("Json Parse Error:", err);
				return;
			}				
			console.log(retObj);
			let vars = Array.from(retObj, (x: Map<string, any>) =>
				new RuntimeVariable(
					x["Name"],
					x["Value"],
					x["Id"],					
					x["Type"],
					x["Size"],
					frameId));
			cb(vars);
        });
	}
	public getLocalVariable(name: string): RuntimeVariable | undefined {
		//TODO: for Set Varible's value
		return undefined;
	}
	public setObject(threadId, frameId,varType,objId,varName,newVal,cb)
	{
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() +
			",frameId=" + frameId.toString()
			+ ",cmd='SetObjectValue'"
			+ ",param=['" + objId+"'"
			+",'"+varType.toString()+"'"
			+ ",'" + varName+"'"
			+ "," + newVal.toString()+ "]"
			+ ")";
		this.Call(code, (retVal) => {
			console.log(retVal);
			try {
				var retObj = JSON.parse(retVal);
			}
			catch (err) {
				console.log("Json Parse Error:", err);
				return;
			}
			console.log(retObj);
			let verifiedValue = 
				new RuntimeVariable(
					varName,
					retObj["Value"],
					retObj["Id"],	
					retObj["Type"],
					retObj["Size"],
					frameId);
			cb(verifiedValue);
		});
	}
	public getObject(threadId, frameId,varType,objId,start,count, cb) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() +
			",frameId=" + frameId.toString()
			+ ",cmd='Object'"
			+ ",param=['"+varType+"'"
			+",'" + objId+"'"
			+ "," + start.toString()
			+ "," + count.toString()+ "]"
			+ ")";
		this.Call(code, (retVal) => {
			console.log(retVal);
			try {
				var retObj = JSON.parse(retVal);
			}
			catch (err) {
				console.log("Json Parse Error:", err);
				return;
			}
			console.log(retObj);
			let vars = Array.from(retObj, (x: Map<string, any>,idx) =>
				new RuntimeVariable(
					x["Name"] === undefined ? (start + idx).toString():x["Name"].toString(),
					x["Value"],
					x["Id"],	
					x["Type"],
					x["Size"],
					frameId));
			cb(vars);
		});
	}
	/**
	 * Return words of the given address range as "instructions"
	 */
	public disassemble(address: number, instructionCount: number): RuntimeDisassembledInstruction[] {

		const instructions: RuntimeDisassembledInstruction[] = [];

		for (let a = address; a < address + instructionCount; a++) {
			if (a >= 0 && a < this.instructions.length) {
				instructions.push({
					address: a,
					instruction: this.instructions[a].name,
					line: this.instructions[a].line
				});
			} else {
				instructions.push({
					address: a,
					instruction: 'nop'
				});
			}
		}

		return instructions;
	}

	// private methods

	private getLine(line?: number): string {
		//return this.sourceLines[line === undefined ? this.currentLine : line].trim();
		return "print('todo')";
	}
	private async GetStartLine(cb){
		let code = "import xdb\nreturn xdb.get_startline("+this._moduleKey.toString()+")";
		this.Call(code, (retVal) => {
			let lineNum = parseInt(retVal);
			cb(lineNum);
        });
	}

	private sendEvent(event: string, ... args: any[]): void {
		setTimeout(() => {
			this.emit(event, ...args);
		}, 0);
	}

	private normalizePathAndCasing(path: string) {
		if (process.platform === 'win32') {
			return path.replace(/\//g, '\\').toLowerCase();
		} else {
			return path.replace(/\\/g, '/');
		}
	}
}
