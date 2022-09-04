/*---------------------------------------------------------
 * https://microsoft.github.io/debug-adapter-protocol/overview
 *--------------------------------------------------------*/

import { EventEmitter } from 'events';


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
	index: number;
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
	private _size: number = 0;
	private _type: string = "";
	private _frameId: number = 0;
	private _reference: number = 0;
	constructor(name: string, value, type: string,size:number,frmId:number) {
		this._name = name;
		this._type = type;
		this._value = value;
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

}

interface Word {
	name: string;
	line: number;
	index: number;
}

export function timeout(ms: number) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

export class XLangRuntime extends EventEmitter {

	private _sourceFile: string = '';
	private _moduleKey: number = 0;
	private _sessionRunning: boolean = false;
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
	private set currentLine(x) {
		this._currentLine = x;
		this.instruction = this.starts[x];
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
	public createScopeRef(varType, frameId,val) {
		let refId = this.nextVarRef++;
		this.varRefMap[refId] = [varType, frameId,val];
		return refId;
	}
	public getScopeRef(refId) {
		return this.varRefMap[refId];
	}

	public close() {
	}
	private async loadSource(file: string): Promise<number> {
		let srcFile = this.normalizePathAndCasing(file);
		let key = this.sourceModuleKeyMap.get(srcFile);
		if (key != undefined) {
			return key;
        }
		let srcFile_x = srcFile.replaceAll('\\', '/');
		let code = "m = load('" + srcFile_x
			+ "')\nreturn m";
		let promise = new Promise((resolve, reject) => {
			this.Call(code, resolve);
		});
		let retVal= await promise as number;
		this.sourceModuleKeyMap.set(srcFile, retVal);
		return retVal;
	}
	private async fetchNotify()
	{
		const https = require('http');
		const options = {
		hostname: 'localhost',
		port: 3141,
		path: '/devops/getnotify',
		method: 'GET'
		};
		const req = https.request(options, res => {
		console.log(`statusCode: ${res.statusCode}`);	
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
						let v = kv["HitBreakpoint"];
						if(v!==undefined){
							this.sendEvent('stopOnBreakpoint');
						}
					}
				}
			}
			else if(strData === "end" || strData === "error")
			{
				this._sessionRunning = false;
				this.sendEvent('end');
			}
			if(this._sessionRunning )
			{
				this.fetchNotify();
			}
		});
		});
	
		req.on('error', error => {
			console.error(error);
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
	 * Start executing the given program.
	 */
	public async start(program: string, stopOnEntry: boolean, debug: boolean): Promise<void> {
		this._sessionRunning = true;
		this.fetchNotify();
		this._sourceFile = this.normalizePathAndCasing(program);
		this._moduleKey = await this.loadSource(this._sourceFile);
		if (this._moduleKey!=0) {
			let code = "tid=threadid()\nmainrun(" + this._moduleKey.toString()
				+ ", onFinish = 'fire(\"devops.dbg\",action=\"end\",tid=${tid})'"
				+ ",stopOnEntry=True)\nreturn True";
			this.Call(code, (ret) => {
				console.log(ret);
				if (debug) {
					//this.verifyBreakpoints(this._sourceFile);
					this.GetStartLine((startLine) => {
						if (stopOnEntry) {
							this.currentLine = startLine - 1;
							this.sendEvent('stopOnEntry');
						} else {
							// we just start to run until we hit a breakpoint, an exception, or the end of the program
							this.continue(false,()=>{ });
						}
					});
				} else {
					this.continue(false, () => { });
				}
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
		hostname: 'localhost',
		port: 3141,
		path: '/devops/run?'+requestargs,
		method: 'GET'
		};
		const req = https.request(options, res => {
		console.log(`statusCode: ${res.statusCode}`);	
		res.on('data', d => {
			var strData = new TextDecoder().decode(d);
			cb(strData);
		});
		});
	
		req.on('error', error => {
		console.error(error);
		});
	
		req.end();
	
	}

	public continue(reverse: boolean,cb) {
		let code = "import xdb\nreturn xdb.command("
			+ this._moduleKey.toString() + ",cmd='Continue')";
		this.Call(code, (retData) => cb());
	}
	public step(instruction: boolean, reverse: boolean,cb:Function) {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() + ",cmd='Step')";
		this.Call(code, (retData) => {
			this.sendEvent('stopOnStep');
			cb();
        });
	}
	public async setBreakPoints(path: string, lines: number[], cb: Function) {
		let mKey = await this.loadSource(this.normalizePathAndCasing(path));
		let code = "import xdb\nreturn xdb.set_breakpoints(" + mKey.toString()
			+ ",[" + lines.join() + "]"
			+ ",path=\"" + path+"\""
			+")";
		this.Call(code, (retData) => {
			var retLines = JSON.parse(retData);
			cb(retLines);
		});
	}
	/**
	 * "Step into" for XLang debug means: go to next character
	 */
	public stepIn(targetId: number | undefined, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() + ",cmd='StepIn')";
		this.Call(code, (retData) => {
			this.sendEvent('stopOnStep');
			cb();
		});
	}

	/**
	 * "Step out" for XLang debug means: go to previous character
	 */
	public stepOut(cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() + ",cmd='StepOut')";
		this.Call(code, (retData) => {
			this.sendEvent('stopOnStep');
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
	public stack(startFrame: number, endFrame: number, cb: Function) {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() + ",cmd='Stack')";
		this.Call(code, (retVal) => {
			console.log(retVal);
			var retObj = JSON.parse(retVal);
			console.log(retObj);
			const frames: IRuntimeStackFrame[] = [];
			// every word of the current line becomes a stack frame.
			for (let i in retObj) {
				let frm = retObj[i];
				let name = frm["name"];
				if (name == "") {
					name = "main";
				}
				const stackFrame: IRuntimeStackFrame = {
					index: frm["index"],
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

	/*
	 * Determine possible column breakpoint positions for the given line.
	 * Here we return the start location of words with more than 8 characters.
	 */
	public getBreakpoints(path: string, line: number): number[] {
		return this.getWords(line, this.getLine(line)).filter(w => w.name.length > 8).map(w => w.index);
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

	public async getGlobalVariables(cancellationToken?: () => boolean ): Promise<RuntimeVariable[]> {

		let a: RuntimeVariable[] = [];

		for (let i = 0; i < 10; i++) {
			//todo:a.push(new RuntimeVariable(`global_${i}`, i));
			if (cancellationToken && cancellationToken()) {
				break;
			}
			await timeout(1000);
		}

		return a;
	}

	public getLocalVariables(frameId,cb){
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() +
			",frameId=" + frameId.toString()+",cmd='Locals')";
		this.Call(code, (retVal) => {
			console.log(retVal);
			var retObj = JSON.parse(retVal);
			console.log(retObj);
			let vars = Array.from(retObj, (x: Map<string, any>) =>
				new RuntimeVariable(
					x["Name"],
					x["Value"],
					x["Type"],
					x["Size"],
					frameId));
			cb(vars);
        });
	}
	public getObject(frameId,objId,start,count, cb) {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() +
			",frameId=" + frameId.toString()
			+ ",cmd='Object'"
			+ ",param=[" + objId.toString()
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
					x["Name"] ==
						undefined ? (start + idx).toString()
						: x["Name"].toString(),
					x["Value"],
					x["Type"],
					x["Size"],
					frameId));
			cb(vars);
		});
	}
	public getLocalVariable(name: string): RuntimeVariable | undefined {
		return this.variables.get(name);
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
		return "print('sss')";
	}

	private getWords(l: number, line: string): Word[] {
		// break line into words
		const WORD_REGEXP = /[a-z]+/ig;
		const words: Word[] = [];
		let match: RegExpExecArray | null;
		while (match = WORD_REGEXP.exec(line)) {
			words.push({ name: match[0], line: l, index: match.index });
		}
		return words;
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
