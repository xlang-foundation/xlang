﻿/*
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

/*---------------------------------------------------------
 * https://microsoft.github.io/debug-adapter-protocol/overview
 *--------------------------------------------------------*/

import { EventEmitter } from 'events';
import * as vscode from 'vscode';
import * as os from 'os';
import * as fs from 'fs';
import * as crypto from 'crypto';

function getTimestamp(): string {
	let date: Date = new Date();
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');

    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const seconds = String(date.getSeconds()).padStart(2, '0');
    const milliseconds = String(date.getMilliseconds()).padStart(3, '0');

    return `[${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${milliseconds}] `;
}

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
	md5: string;
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

	private _runFile: string = '';
	public get runFile(){
		return this._runFile;
	}
	private _needSrvPath : boolean = false;
	private _outputChannel : vscode.OutputChannel;
	private _sourceFile: string = '';
	private _moduleKey: number = 0;
	private _sessionRunning: boolean = false;
	private _srvaddress:string ="localhost";
	private _srvPort:number =3142;
	public get sourceFile() {
		return this._sourceFile;
	}

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

	public runMode = "";

	// This is the next instruction that will be 'executed'
	public instruction= 0;

	// all instruction breakpoint addresses
	private instructionBreakpoints = new Set<number>();


	private breakAddresses = new Map<string, string>();

	private namedException: string | undefined;
	private otherExceptions = false;


	constructor() {
		super();
		this._outputChannel = vscode.window.createOutputChannel("XLang Output");
    	this._outputChannel.show(true);
    	this.addOutput("XLang extension active");
	}

	public addOutput(val : string)
	{
		this._outputChannel.show(true)
		this._outputChannel.appendLine(`${getTimestamp()}${val}`);
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

	public close(closeXlang: boolean) {
		if (closeXlang){
			this.terminateXlang();
		}
		else{
			this.setDebug(false);
		}
		this._sourceFile = '';
		this._moduleKey = 0;
		this._sessionRunning = false;
		this.reqNotify?.abort();
	}
	public async loadSource(file: string): Promise<number> {
		let bIsX = file.endsWith(".x") || file.endsWith(".X");
		let code;
		let content;
		let srcArg = {};
		if (bIsX)
		{
			content = fs.readFileSync(file, 'utf-8').replace(/\r\n/g, '\n');
			const hash = crypto.createHash('md5');
			hash.update(content);
			let md5 = hash.digest('hex');
			srcArg['src'] = content;
			srcArg['md5'] = md5;
			if (this._needSrvPath)
			{
				file = await vscode.window.showInputBox({value: file, prompt: "platform not match or file not exists, input remote path of current file to debug", placeHolder: file});
			}
			code = "m = load('" + file + "','" + this.runMode + "','" + md5 + "')\nreturn m";
		}
		else
		{
			if (this._needSrvPath)
			{
				file = await vscode.window.showInputBox({value: file, prompt: "platform not match or file not exists, input remote path of current file to run", placeHolder: file});
			}
			this.addOutput(`run file: "${file}"`);
			this._runFile = file;
			code = "import xdb\nreturn xdb.run_file(\"" + file + "\")";
		}
		let loadRet = new Promise((resolve, reject) => {
			this.Call(code, srcArg, resolve);
		});
		let retVal;
		if (bIsX)
		{
			retVal = await loadRet as number;
		}
		else
		{
			retVal = await loadRet as string;
			if (retVal.length > 0 && (retVal.startsWith("http:") || retVal.startsWith("https:")))
			{
				this.addOutput(`output url: "${retVal}"`);
			}
			return -1; // return -1 for run file
		}
		this._sourceFile = file;
		this._moduleKey = retVal; // if 0, module is previous loaded, do not run it again
		return retVal;
	}
	private tryTimes = 1;
	private tryCount = 5;
	public async checkStarted(path : string, md5 : string)
	{
		vscode.window.showInformationMessage(`try connecting to a xlang dbg server at ${this.serverAddress}:${this.serverPort}, try ${this.tryTimes}`);
		const https = require('http');
		const querystring = require('querystring');
		const parameters = {
			'path': path,
			'md5': md5,
			'platform': process.platform === "win32" ? "windows" : "not_windows"
		};
		const requestargs = querystring.stringify(parameters);
		const options = {
			hostname: this._srvaddress,
			port: this._srvPort,
			path: '/devops/checkStarted?' + requestargs,
			method: 'GET',
			timeout: 2000
		};
		const req = https.request(options, res => {
			res.on('data', d => {
				this._needSrvPath = d.toString() === "need_path"
			});
			this.sendEvent('xlangStarted', true);
			vscode.window.showInformationMessage(`connecting to a xlang dbg server at ${this.serverAddress}:${this.serverPort} successed`);
			this.tryTimes = 1;
		});
	
		req.on('error', error => {
			if (error.code === 'ECONNREFUSED' && this.tryTimes <= this.tryCount)
			{
				var thisObj = this;
				setTimeout(function() {
					thisObj.checkStarted(path, md5);
				}, 2000);
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

	private reqTerminate;
	public async terminateXlang()
	{
		
		this.addOutput("terminate xlang server");
		if (this._runFile.length > 0)
		{
			let code = "import xdb\nreturn xdb.stop_file(\"" + this._runFile + "\")";
			let promise = new Promise((resolve, reject) => {
				this.Call(code, undefined, resolve);
			});
			this._runFile = "";
			await promise as number;;
		}

		const https = require('http');
		const options = {
			hostname: this._srvaddress,
			port: this._srvPort,
			path: '/devops/terminate',
			method: 'GET'
		};

		this.reqTerminate = https.request(options);
		this.reqTerminate.end();
	}

	private reqNotify;
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
		this.reqNotify = https.request(options, res => {
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
								else if(kv.hasOwnProperty("BreakpointMd5")){
									this.sendEvent('breakpointState', kv["BreakpointMd5"], kv["line"], kv["actualLine"]);
								}
								else if(kv.hasOwnProperty("ModuleLoaded")){
									this.sendEvent('moduleLoaded', kv["ModuleLoaded"], kv["md5"]);
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
	
		this.reqNotify.on('error', error => {
			console.error("fetchNotify->",error);
			if ((error?.code === "ECONNRESET" || error?.code === "ECONNREFUSED") && this._sessionRunning )
			{
				vscode.window.showErrorMessage(`disconnect from xlang dbg server at ${this.serverAddress}:${this.serverPort} debugging stopped`, { modal: true }, "ok");
				this.sendEvent('end');
			}
			else if(this._sessionRunning )
			{
				var thisObj = this;
				setTimeout(function() {
					thisObj.fetchNotify();
				}, 100);
			}
		});
		this.reqNotify.end();
	}
	/**
	 * Start executing the loaded program.
	 */
	public async start(stopOnEntry: boolean, debug: boolean): Promise<void> {
		this._sessionRunning = true;
		this.fetchNotify();
		if (this._moduleKey!=0) // new created module, run it
		{
			this.addOutput(`entry source file is new loaded, run it: "${this._sourceFile}"`);	
			let code = "tid=threadid()\nmainrun(" + this._moduleKey.toString()
				+ ", onFinish = 'fire(\"devops.dbg\",action=\"end\",tid=${tid})'"
				+ ",stopOnEntry=True)\nreturn True";
			this.Call(code, undefined, (ret) => {
				console.log(ret);
			});
		}
		else
		{
			this.addOutput(`entry source file has loaded previously: "${this._sourceFile}"`);	
		}
	}
	private Call(code, srcArg, cb?)
	{
		const https = require('http');
		let bufCode = Buffer.from(code);
		let totalLength = 4 + bufCode.length;
		let bufSrc;
		let bufMd5;
		if (srcArg !== undefined) {
			bufSrc = Buffer.from(srcArg.src);
			totalLength += 4 + bufSrc.length;
			bufMd5 = Buffer.from(srcArg.md5);
			totalLength += 4 + bufMd5.length;
		}
		let buffer = Buffer.alloc(totalLength);
		let offset = 0;
		buffer.writeUInt32BE(bufCode.length, 0);
		offset += 4
		bufCode.copy(buffer, offset);
		if (srcArg !== undefined)
		{
			offset += bufCode.length;
			buffer.writeUInt32BE(bufSrc.length, offset);
			offset += 4
			bufSrc.copy(buffer, offset);
			offset += bufSrc.length;
			buffer.writeUInt32BE(bufMd5.length, offset);
			offset += 4
			bufMd5.copy(buffer, offset);
		}

		const options = {
			hostname: this._srvaddress,
			port: this._srvPort,
			path: '/devops/run',
			method: 'POST',
			headers: {
				'Content-Type': 'application/octet-stream',
				'Content-Length': totalLength
			}
		};
		const req = https.request(options, res => {
			console.log(`statusCode: ${res.statusCode}`);
			let allData = "";
			res.on('end', () => {
				cb?.(allData);
			});
			res.on('data', d => {
				let strData = new TextDecoder().decode(d);
				allData +=strData;
			});
		});
	
		req.on('error', error => {
			console.error(error);
		});
		req.write(buffer);
		req.end();
	}

	public continue(reverse: boolean, threadId: number, cb) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Continue')";
		this.Call(code, undefined, (retData) => cb());
	}
	public step(instruction: boolean, reverse: boolean, threadId: number, cb:Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Step')";
		this.Call(code, undefined, (retData) => {
			//this.sendEvent('stopOnStep');
			cb();
        });
	}
	public async setBreakPoints(path: string, md5: string, lines: number[], cb: Function) {
		let path_x = path.replaceAll('\\', '/');
		let code = "import xdb\nreturn xdb.set_breakpoints(\"" + path_x + "\",\"" + md5 + "\",[" + lines.join() + "])";
		this.Call(code, undefined, (retData) => {
			var retLines = JSON.parse(retData);
			cb(retLines);
		});
	}
	/**
	 * "Step into" for XLang debug means: go to next character
	 */
	public stepIn(threadId:number, targetId: number | undefined, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='StepIn')";
		this.Call(code, undefined, (retData) => {
			//this.sendEvent('stopOnStep');
			cb();
		});
	}

	/**
	 * "Step out" for XLang debug means: go to previous character
	 */
	public stepOut(threadId:number, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='StepOut')";
		this.Call(code, undefined, (retData) => {
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
	
	private setDebug(bDebug : boolean)
	{
		if (bDebug)
			this.addOutput("enable xlang server debug");
		else
			this.addOutput("disable xlang server debug");
		let code = "import xdb\nreturn xdb.set_debug(" + (bDebug ? '1' : '0') +")";
		this.Call(code);
	}

	public getThreads(cb: Function)
	{
		let code = "import xdb\nreturn xdb.get_threads()";
		this.Call(code, undefined, (retVal) => {
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
		this.Call(code, undefined, (retVal) => {
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
					md5: frm["md5"],
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
		this.Call(code, undefined, (retVal) => {
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
		this.Call(code, undefined, (retVal) => {
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
		this.Call(code, undefined, (retVal) => {
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
		this.Call(code, undefined, (retVal) => {
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
		this.Call(code, undefined, (retVal) => {
			let lineNum = parseInt(retVal);
			cb(lineNum);
        });
	}

	private sendEvent(event: string, ... args: any[]): void {
		setTimeout(() => {
			this.emit(event, ...args);
		}, 0);
	}

	public normalizePathAndCasing(path: string) {
		return path.replace(/\\/g, '/');
	}
}
