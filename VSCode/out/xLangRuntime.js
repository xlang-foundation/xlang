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
exports.XLangRuntime = exports.timeout = exports.RuntimeVariable = void 0;
/*---------------------------------------------------------
 * https://microsoft.github.io/debug-adapter-protocol/overview
 *--------------------------------------------------------*/
const events_1 = require("events");
const vscode = require("vscode");
const fs = require("fs");
const crypto = require("crypto");
function getTimestamp() {
    let date = new Date();
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const seconds = String(date.getSeconds()).padStart(2, '0');
    const milliseconds = String(date.getMilliseconds()).padStart(3, '0');
    return `[${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${milliseconds}] `;
}
class RuntimeVariable {
    constructor(name, value, id, type, size, frmId) {
        this._name = "";
        this._value = null;
        this._id = "";
        this._size = 0;
        this._type = "";
        this._frameId = 0;
        this._reference = 0;
        this._name = name;
        this._type = type;
        this._value = value;
        this._id = id;
        this._size = size;
        this._frameId = frmId;
    }
    get FrameId() {
        return this._frameId;
    }
    get Name() {
        return this._name;
    }
    set Name(nm) {
        this._name = nm;
    }
    get Size() { return this._size; }
    get reference() {
        return this._reference;
    }
    set reference(r) {
        this._reference = r;
    }
    get Type() {
        return this._type;
    }
    set Type(t) {
        this._type = t;
    }
    get Val() {
        return this._value;
    }
    set Val(v) {
        this._value = v;
    }
    get Id() {
        return this._id;
    }
    set Id(v) {
        this._id = v;
    }
}
exports.RuntimeVariable = RuntimeVariable;
function timeout(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}
exports.timeout = timeout;
class XLangRuntime extends events_1.EventEmitter {
    constructor() {
        super();
        this._runFile = '';
        this._needSrvPath = false;
        this._sourceFile = '';
        this._moduleKey = 0;
        this._runModule = false;
        this._breakPointThreadId = 0;
        this._sessionRunning = false;
        this._srvaddress = "localhost";
        this._srvPort = 3142;
        this.instructions = [];
        this.starts = [];
        // This is the next line that will be 'executed'
        this._currentLine = 0;
        this.runMode = "";
        // This is the next instruction that will be 'executed'
        this.instruction = 0;
        // all instruction breakpoint addresses
        this.instructionBreakpoints = new Set();
        this.breakAddresses = new Map();
        this.nextVarRef = 1;
        this.varRefMap = new Map();
        this.tryTimes = 1;
        this.tryCount = 5;
        this._outputChannel = vscode.window.createOutputChannel("XLang Output");
        this._outputChannel.show(true);
        this.addOutput("XLang extension active");
    }
    get runFile() {
        return this._runFile;
    }
    get sourceFile() {
        return this._sourceFile;
    }
    get currentLine() {
        return this._currentLine;
    }
    set currentLine(x) {
        this._currentLine = x;
        this.instruction = this.starts[x];
    }
    get serverPort() {
        return this._srvPort;
    }
    set serverPort(port) {
        this._srvPort = port;
    }
    get serverAddress() {
        return this._srvaddress;
    }
    set serverAddress(addr) {
        this._srvaddress = addr;
    }
    addOutput(val) {
        this._outputChannel.show(true);
        this._outputChannel.appendLine(`${getTimestamp()}${val}`);
    }
    createScopeRef(varType, frameId, val, id) {
        let refId = this.nextVarRef++;
        this.varRefMap[refId] = [varType, frameId, val, id];
        return refId;
    }
    getScopeRef(refId) {
        return this.varRefMap[refId];
    }
    close(closeXlang) {
        if (closeXlang) {
            this.terminateXlang();
        }
        else {
            this.setDebug(false);
        }
        this._sourceFile = '';
        this._moduleKey = 0;
        this._sessionRunning = false;
        this.reqNotify?.abort();
    }
    async loadSource(file) {
        let bIsX = file.endsWith(".x") || file.endsWith(".X");
        let code;
        let content;
        let srcArg = {};
        if (bIsX) {
            content = fs.readFileSync(file, 'utf-8').replace(/\r\n/g, '\n');
            const hash = crypto.createHash('md5');
            hash.update(content);
            let md5 = hash.digest('hex');
            srcArg['src'] = content;
            srcArg['md5'] = md5;
            if (this._needSrvPath) {
                const newFile = await vscode.window.showInputBox({ value: file, prompt: "platform not match or file not exists, input remote path of current file to debug", placeHolder: file });
                if (newFile) {
                    file = newFile;
                }
            }
            code = "m = load('" + file + "','" + this.runMode + "','" + md5 + "')\nreturn m";
        }
        else {
            if (this._needSrvPath) {
                const newFile = await vscode.window.showInputBox({ value: file, prompt: "platform not match or file not exists, input remote path of current file to run", placeHolder: file });
                if (newFile) {
                    file = newFile;
                }
            }
            this.addOutput(`run file: "${file}"`);
            this._runFile = file;
            code = "import xdb\nreturn xdb.run_file(\"" + file + "\")";
        }
        let loadRet = new Promise((resolve, reject) => {
            this.Call(code, srcArg, resolve);
        });
        let retVal;
        if (bIsX) {
            retVal = JSON.parse(await loadRet);
        }
        else {
            retVal = await loadRet;
            if (retVal.length > 0 && (retVal.startsWith("http:") || retVal.startsWith("https:"))) {
                this.addOutput(`output url: "${retVal}"`);
            }
            return -1; // return -1 for run file
        }
        this._sourceFile = file;
        this._runModule = retVal[0] === 1;
        this._moduleKey = retVal[1];
        return retVal[0]; // if 0, module is previous loaded, do not run it again
    }
    async checkStarted(path, md5) {
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
                this._needSrvPath = d.toString() === "need_path";
            });
            this.sendEvent('xlangStarted', true);
            vscode.window.showInformationMessage(`connecting to a xlang dbg server at ${this.serverAddress}:${this.serverPort} successed`);
            this.tryTimes = 1;
        });
        req.on('error', error => {
            if (error.code === 'ECONNREFUSED' && this.tryTimes <= this.tryCount) {
                var thisObj = this;
                setTimeout(function () {
                    thisObj.checkStarted(path, md5);
                }, 2000);
                ++this.tryTimes;
            }
            else {
                this.tryTimes = 1;
                this.sendEvent('xlangStarted', false);
            }
        });
        req.end();
    }
    async terminateXlang() {
        this.addOutput("terminate xlang server");
        if (this._runFile.length > 0) {
            let code = "import xdb\nreturn xdb.stop_file(\"" + this._runFile + "\")";
            let promise = new Promise((resolve, reject) => {
                this.Call(code, undefined, resolve);
            });
            this._runFile = "";
            await promise;
            ;
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
    async fetchNotify() {
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
                var strData = d.toString('utf8');
                console.log(strData);
                let tagNoti = "$notify$";
                if (strData.startsWith(tagNoti)) {
                    var param = strData.substring(tagNoti.length);
                    var notis = JSON.parse(param);
                    if (notis) {
                        for (let n in notis) {
                            let kv = notis[n];
                            if (kv.hasOwnProperty("HitBreakpoint")) {
                                this._breakPointThreadId = kv["threadId"];
                                this.sendEvent('stopOnBreakpoint', this._breakPointThreadId);
                            }
                            else if (kv.hasOwnProperty("StopOnEntry")) {
                                this.sendEvent('stopOnEntry', kv["StopOnEntry"]);
                            }
                            else if (kv.hasOwnProperty("StopOnStep")) {
                                this.sendEvent('stopOnStep', kv["StopOnStep"]);
                            }
                            else if (kv.hasOwnProperty("ThreadStarted")) {
                                this.sendEvent('threadStarted', kv["ThreadStarted"]);
                            }
                            else if (kv.hasOwnProperty("ThreadExited")) {
                                this.sendEvent('threadExited', kv["ThreadExited"]);
                            }
                            else if (kv.hasOwnProperty("BreakpointMd5")) {
                                this.sendEvent('breakpointState', kv["BreakpointMd5"], kv["line"], kv["actualLine"]);
                            }
                            else if (kv.hasOwnProperty("ModuleLoaded")) {
                                this.sendEvent('moduleLoaded', kv["ModuleLoaded"], kv["md5"]);
                            }
                        }
                    }
                }
                else if (strData === "end" || strData === "error") {
                    this._sessionRunning = false;
                    this.sendEvent('end');
                }
            });
            res.on('end', () => {
                if (this._sessionRunning) {
                    this.fetchNotify();
                }
            });
        });
        this.reqNotify.on('error', error => {
            console.error("fetchNotify->", error);
            if ((error?.code === "ECONNRESET" || error?.code === "ECONNREFUSED") && this._sessionRunning) {
                vscode.window.showErrorMessage(`disconnect from xlang dbg server at ${this.serverAddress}:${this.serverPort} debugging stopped`, { modal: true }, "ok");
                this.sendEvent('end');
            }
            else if (this._sessionRunning) {
                var thisObj = this;
                setTimeout(function () {
                    thisObj.fetchNotify();
                }, 100);
            }
        });
        this.reqNotify.end();
    }
    /**
     * Start executing the loaded program.
     */
    async start(stopOnEntry, debug) {
        this._sessionRunning = true;
        this.fetchNotify();
        if (this._runModule) // new created module, run it
         {
            this.addOutput(`entry source file is new loaded, run it: "${this._sourceFile}"`);
            let code = "tid=threadid()\nmainrun(" + this._moduleKey.toString()
                + ", onFinish = 'fire(\"devops.dbg\",action=\"end\",tid=${tid})'"
                + ",stopOnEntry=True)\nreturn True";
            this.Call(code, undefined, (ret) => {
                console.log(ret);
            });
        }
        else {
            this.addOutput(`entry source file has loaded previously: "${this._sourceFile}"`);
        }
    }
    Call(code, srcArg, cb) {
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
        offset += 4;
        bufCode.copy(buffer, offset);
        if (srcArg !== undefined) {
            offset += bufCode.length;
            buffer.writeUInt32BE(bufSrc.length, offset);
            offset += 4;
            bufSrc.copy(buffer, offset);
            offset += bufSrc.length;
            buffer.writeUInt32BE(bufMd5.length, offset);
            offset += 4;
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
                let strData = d.toString('utf8');
                allData += strData;
            });
        });
        req.on('error', error => {
            console.error(error);
        });
        req.write(buffer);
        req.end();
    }
    continue(reverse, threadId, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Continue')";
        this.Call(code, undefined, (retData) => cb());
    }
    step(instruction, reverse, threadId, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Step')";
        this.Call(code, undefined, (retData) => {
            //this.sendEvent('stopOnStep');
            cb();
        });
    }
    async setBreakPoints(path, md5, lines, cb) {
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
    stepIn(threadId, targetId, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='StepIn')";
        this.Call(code, undefined, (retData) => {
            //this.sendEvent('stopOnStep');
            cb();
        });
    }
    /**
     * "Step out" for XLang debug means: go to previous character
     */
    stepOut(threadId, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='StepOut')";
        this.Call(code, undefined, (retData) => {
            //this.sendEvent('stopOnStep');
            cb();
        });
    }
    getStepInTargets(frameId) {
        const line = this.getLine();
        const words = this.getWords(this.currentLine, line);
        // return nothing if frameId is out of range
        if (frameId < 0 || frameId >= words.length) {
            return [];
        }
        const { name, index } = words[frameId];
        // make every character of the frame a potential "step in" target
        return name.split('').map((c, ix) => {
            return {
                id: index + ix,
                label: `target: ${c}`
            };
        });
    }
    getWords(currentLine, line) {
        const words = [];
        // simple implementation to split by words
        const parts = line.split(/(\W+)/);
        let index = 0;
        for (const part of parts) {
            if (part.trim().length > 0 && /\w+/.test(part)) {
                words.push({ name: part, line: currentLine, index: index });
            }
            index += part.length;
        }
        return words;
    }
    setDebug(bDebug) {
        if (bDebug)
            this.addOutput("enable xlang server debug");
        else
            this.addOutput("disable xlang server debug");
        let code = "import xdb\nreturn xdb.set_debug(" + (bDebug ? '1' : '0') + ")";
        this.Call(code, undefined);
    }
    getThreads(cb) {
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
                threads.push({ id: t["id"], name: t["name"].toString() });
            }
            cb(threads);
        });
    }
    stack(threadId, startFrame, endFrame, cb) {
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
            const frames = [];
            // every word of the current line becomes a stack frame.
            for (let i in retObj) {
                let frm = retObj[i];
                let name = frm["name"];
                if (name === "") {
                    name = "main";
                }
                const stackFrame = {
                    id: frm["id"],
                    name: name,
                    file: frm["file"],
                    md5: frm["md5"],
                    line: frm["line"] - 1,
                    column: frm["column"]
                };
                frames.push(stackFrame);
            }
            let stk = {
                frames: frames,
                count: retObj ? retObj.length : 0
            };
            cb(stk);
        });
    }
    setDataBreakpoint(address, accessType) {
        const x = accessType === 'readWrite' ? 'read write' : accessType;
        const t = this.breakAddresses.get(address);
        if (t) {
            if (t !== x) {
                this.breakAddresses.set(address, 'read write');
            }
        }
        else {
            this.breakAddresses.set(address, x);
        }
        return true;
    }
    clearAllDataBreakpoints() {
        this.breakAddresses.clear();
    }
    setExceptionsFilters(namedException, otherExceptions) {
        // this.namedException = namedException;
        // this.otherExceptions = otherExceptions;
    }
    setInstructionBreakpoint(address) {
        this.instructionBreakpoints.add(address);
        return true;
    }
    clearInstructionBreakpoints() {
        this.instructionBreakpoints.clear();
    }
    getBreakpoints(path, line) {
        return [];
    }
    setBreakPoint(path, line) {
        return { verified: false, line: line, id: 0 };
    }
    clearBreakPoint(path, line) {
        return { id: 0 };
    }
    getGlobalVariables(threadId, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() + ",cmd='Globals')";
        this.Call(code, undefined, (retVal) => {
            console.log(retVal);
            var retObj = JSON.parse(retVal);
            console.log(retObj);
            let vars = Array.from(retObj || [], (x) => new RuntimeVariable(x["Name"], x["Value"], x["Id"], x["Type"], x["Size"], 0));
            cb(vars);
        });
    }
    getLocalVariables(threadId, frameId, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() +
            ",frameId=" + frameId.toString() + ",cmd='Locals')";
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
            let vars = Array.from(retObj || [], (x) => new RuntimeVariable(x["Name"], x["Value"], x["Id"], x["Type"], x["Size"], frameId));
            cb(vars);
        });
    }
    getLocalVariable(name) {
        //TODO: for Set Varible's value
        return undefined;
    }
    setObject(threadId, frameId, varType, objId, varName, newVal, cb) {
        let code = "import xdb\nreturn xdb.command(" + threadId.toString() +
            ",frameId=" + frameId.toString()
            + ",cmd='SetObjectValue'"
            + ",param=['" + objId + "'"
            + ",'" + varType.toString() + "'"
            + ",'" + varName + "'"
            + "," + newVal.toString() + "]"
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
            let verifiedValue = new RuntimeVariable(varName, retObj["Value"], retObj["Id"], retObj["Type"], retObj["Size"], frameId);
            cb(verifiedValue);
        });
    }
    getObject(threadId, frameId, varType, objId, start, count, cb) {
        let code = "import xdb\nreturn xdb.command(" + (threadId ? threadId.toString() : this._breakPointThreadId.toString())
            + ",frameId=" + frameId.toString()
            + ",moduleKey=" + this._moduleKey.toString()
            + ",cmd='Object'"
            + ",param=['" + varType + "'"
            + ",'" + objId + "'"
            + "," + start.toString()
            + "," + count.toString() + "]"
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
            let vars = Array.from(retObj, (x, idx) => new RuntimeVariable(x["Name"] === undefined ? (start + idx).toString() : x["Name"].toString(), x["Value"], x["Id"], x["Type"], x["Size"], frameId));
            cb(vars);
        });
    }
    evaluate(expression, frameId, cb) {
        let code = "import xdb\nreturn xdb.command(" + (this._breakPointThreadId || 0).toString() +
            ",frameId=" + (frameId || 0).toString() +
            ",cmd='Eval',param='" + expression.replace(/'/g, "\\'") + "')"; // escape single quote
        this.Call(code, undefined, (retVal) => {
            console.log(retVal);
            try {
                if (retVal === "false" || retVal === "") {
                    cb(undefined);
                    return;
                }
                var retObj = JSON.parse(retVal);
                // retObj is a Dict: Name, Type, Value, etc.
                let verifiedValue = new RuntimeVariable(retObj["Name"], retObj["Value"], retObj["Id"], retObj["Type"], retObj["Size"], frameId);
                cb(verifiedValue);
            }
            catch (err) {
                console.log("Json Parse Error:", err);
                cb(undefined);
            }
        });
    }
    /**
     * Return words of the given address range as "instructions"
     */
    disassemble(address, instructionCount) {
        const instructions = [];
        for (let a = address; a < address + instructionCount; a++) {
            if (a >= 0 && a < this.instructions.length) {
                instructions.push({
                    address: a,
                    instruction: this.instructions[a].name,
                    line: this.instructions[a].line
                });
            }
            else {
                instructions.push({
                    address: a,
                    instruction: 'nop'
                });
            }
        }
        return instructions;
    }
    // private methods
    getLine(line) {
        //return this.sourceLines[line === undefined ? this.currentLine : line].trim();
        return "print('todo')";
    }
    sendEvent(event, ...args) {
        setTimeout(() => {
            this.emit(event, ...args);
        }, 0);
    }
    normalizePathAndCasing(path) {
        return path.replace(/\\/g, '/');
    }
}
exports.XLangRuntime = XLangRuntime;
//# sourceMappingURL=xLangRuntime.js.map