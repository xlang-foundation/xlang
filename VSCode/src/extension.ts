'use strict';

import * as Net from 'net'
import * as vscode from 'vscode';
import { activateXLangDebug} from './activateXLangDebug';


class CallInfo {
    private _inData;
    private _cb;
    constructor(indata, cb) {
        this._inData = indata;
        this._cb = cb;
    }
    public get cb() {
        return this._cb;
    }
    public get Input() {
        return this._inData;
    }
}
export class XlangDevOps {
    private connected: boolean = false;
    private client?: any;
    private canSent: boolean = false;
    private callQ: CallInfo[]=[];
    constructor() {
    }
    Close() {
        if (this.client) {
            this.callQ = [];
            this.client.destroy();
            this.client = null;
            this.connected = false;
            this.canSent = false;
        }
    }
    Connect() {
        var This = this;
        try {
            this.client = Net.connect("\\\\.\\pipe\\x.devops", function () {
                This.connected = true;
                This.canSent = true;
                This.Send();
            }).on('data', function (data) {
                //skip the first 4 bytes as length
                let outData = data.toString('utf8', 4);
                let callData = This.callQ.shift();
                callData?.cb(outData);
                //send next one if have
                This.canSent = true;
                This.Send();
            }).on('end', function () {
                This.callQ = [];
                This.canSent = false;
                This.connected = false;
            }).on('error', function (error) {
                console.log(error);
            });
        } catch (e: any) {
            console.log(e.message);
        }
    }
    Start() {
        this.Connect();
        return true;
    }
    Send() {
        if (this.client == null) {
            return false;
        }
        if (!this.canSent) {
            return false;
        }
        if (this.callQ.length == 0) {
            return false;
        }
        const callData = this.callQ[0];
        let data = callData.Input;
        const header = new Uint32Array(1);
        header[0] = data.length;
        if (!this.client.write(new Uint8Array(header.buffer))) {
            return false;
        }
        if (!this.client.write(data)) {
            return false;
        }
        this.canSent = false;
        return true;
    }
    IsConnected() {
        return this.connected;
    }

    Call(indata, cb) {
        this.callQ.push(new CallInfo(indata, cb));
        this.Send();
    }
    
}

export function activate(context: vscode.ExtensionContext) {
    activateXLangDebug(context);
}

export function deactivate() {
	// nothing to do
}
