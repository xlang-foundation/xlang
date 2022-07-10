/*---------------------------------------------------------
 * https://microsoft.github.io/debug-adapter-protocol/overview
 *--------------------------------------------------------*/
import { EventEmitter } from 'events';
import { XlangDevOps } from './extension';


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

interface IRuntimeStack {
	count: number;
	frames: IRuntimeStackFrame[];
}

interface RuntimeDisassembledInstruction {
	address: number;
	instruction: string;
	line?: number;
}
export class RuntimeVariable {
	private _name: string = "";
	private _value: any = null;
	private _type: string = "";
	private _frameId: number = 0;
	private _reference: number = 0;
	constructor(name: string, value, type: string,frmId:number) {
		this._name = name;
		this._type = type;
		this._value = value;
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

	private _xlangDevOps?:XlangDevOps;
	private _sourceFile: string = '';
	private _moduleKey: number = 0;
	public get sourceFile() {
		return this._sourceFile;
	}

	private variables = new Map<string, RuntimeVariable>();

	// the contents (= lines) of the one and only file
	private sourceLines: string[] = [];
	private instructions: Word[] = [];
	private starts: number[] = [];
	private ends: number[] = [];

	// This is the next line that will be 'executed'
	private _currentLine = 0;
	private get currentLine() {
		return this._currentLine;
	}
	private set currentLine(x) {
		this._currentLine = x;
		this.instruction = this.starts[x];
	}
	private currentColumn: number | undefined;

	// This is the next instruction that will be 'executed'
	public instruction= 0;

	// maps from sourceFile to array of IRuntimeBreakpoint
	private breakPoints = new Map<string, IRuntimeBreakpoint[]>();

	// all instruction breakpoint addresses
	private instructionBreakpoints = new Set<number>();

	// since we want to send breakpoint events, we will assign an id to every event
	// so that the frontend can match events with breakpoints.
	private breakpointId = 1;

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
	checkConnection()
	{
		if(this._xlangDevOps == undefined)
		{
			this._xlangDevOps = new XlangDevOps();
			this._xlangDevOps.Start();
		}
	}
	public close() {
		if (this._xlangDevOps) {
			this._xlangDevOps.Close();
			this._xlangDevOps = undefined;
        }
    }
	/**
	 * Start executing the given program.
	 */
	public async start(program: string, stopOnEntry: boolean, debug: boolean): Promise<void> {
		this.checkConnection();
		if (this._sourceFile !== program) {
			this._sourceFile = this.normalizePathAndCasing(program);
			var srcFile = this._sourceFile.replaceAll('\\', '/');
			let code = "m = load('" + srcFile + "')\nmainrun(m,stopOnEntry=True)\nreturn m";
			this.Call(code, (ret) => {
				console.log(ret);
				this._moduleKey = ret;
				if (debug) {
					//await this.verifyBreakpoints(this._sourceFile);
					this.GetStartLine((startLine) => {
						if (stopOnEntry) {
							this.currentLine = startLine - 1;
							this.sendEvent('stopOnEntry');
						} else {
							// we just start to run until we hit a breakpoint, an exception, or the end of the program
							this.continue(false);
						}
					});
				} else {
					this.continue(false);
				}
			});
		}
	}

	/**
	 * Continue execution to the end/beginning.
	 */
	public continue(reverse: boolean) {

	}
	private Call(code,cb)
	{
		this._xlangDevOps.Call(code, cb);
	}

	public step(instruction: boolean, reverse: boolean,cb:Function) {
		let code = "import xdb\nreturn xdb.command(" + this._moduleKey.toString() + ",cmd='Step')";
		this.Call(code, (retData) => {
			var nextLine = parseInt(retData);
			this.currentLine = nextLine - 1;
			this.sendEvent('stopOnStep');
			cb();
        });
	}

	/**
	 * "Step into" for XLang debug means: go to next character
	 */
	public stepIn(targetId: number | undefined) {
		if (typeof targetId === 'number') {
			this.currentColumn = targetId;
			this.sendEvent('stopOnStep');
		} else {
			if (typeof this.currentColumn === 'number') {
				if (this.currentColumn <= this.sourceLines[this.currentLine].length) {
					this.currentColumn += 1;
				}
			} else {
				this.currentColumn = 1;
			}
			this.sendEvent('stopOnStep');
		}
	}

	/**
	 * "Step out" for XLang debug means: go to previous character
	 */
	public stepOut() {
		if (typeof this.currentColumn === 'number') {
			this.currentColumn -= 1;
			if (this.currentColumn === 0) {
				this.currentColumn = undefined;
			}
		}
		this.sendEvent('stopOnStep');
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
					file: this._sourceFile,
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

	/*
	 * Set breakpoint in file with given line.
	 */
	public async setBreakPoint(path: string, line: number): Promise<IRuntimeBreakpoint> {
		path = this.normalizePathAndCasing(path);

		const bp: IRuntimeBreakpoint = { verified: false, line, id: this.breakpointId++ };
		let bps = this.breakPoints.get(path);
		if (!bps) {
			bps = new Array<IRuntimeBreakpoint>();
			this.breakPoints.set(path, bps);
		}
		bps.push(bp);

		//await this.verifyBreakpoints(path);

		return bp;
	}

	/*
	 * Clear breakpoint in file with given line.
	 */
	public clearBreakPoint(path: string, line: number): IRuntimeBreakpoint | undefined {
		const bps = this.breakPoints.get(this.normalizePathAndCasing(path));
		if (bps) {
			const index = bps.findIndex(bp => bp.line === line);
			if (index >= 0) {
				const bp = bps[index];
				bps.splice(index, 1);
				return bp;
			}
		}
		return undefined;
	}

	public clearBreakpoints(path: string): void {
		this.breakPoints.delete(this.normalizePathAndCasing(path));
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
				new RuntimeVariable(x["Name"], x["Value"], x["Type"], frameId));
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
