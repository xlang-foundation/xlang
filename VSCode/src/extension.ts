'use strict';

import * as Net from 'net'
import * as vscode from 'vscode';
import { activateXLangDebug} from './activateXLangDebug';

var L = console.log;

export class XlangDevOps {
    private connected: boolean = false;
    private onConnected?: (value: any) => void;
    private onData?: (data) => void;
    private onEnd?: () => void;
    private client?: any;
    constructor() {
    }
    Connect() {
        var This = this;
        try {
            this.client = Net.connect("\\\\.\\pipe\\x.devops", function () {
                This.connected = true;
                if (This.onConnected != null) {
                    This.onConnected(true);
                }
            }).on('data', function (data) {
                if (This.onData != null) {
                    This.onData(data);
                }
            }).on('end', function () {
                if (This.onEnd != null) {
                    This.onEnd();
                }
            }).on('error', function (error) {
                L(error);
            });
        } catch (e: any) {
            L(e.message);
        }
    }
    async Start() {
        var This = this;
        await new Promise((resolve, reject) => {
            This.onConnected = resolve;
            This.onEnd = reject;
            This.Connect();
        });
        return this.connected;
    }
    async Send(data) {
        if (this.client == null) {
            return false;
        }
        if (!this.connected) {
            var This = this;
            await new Promise((resolve, reject) => {
                This.onConnected = resolve;
                This.onEnd = reject;
            });
            if (!this.connected) {
                return false;
            }
        }
        const header = new Uint32Array(1);
        header[0] = data.length;
        if (!this.client.write(new Uint8Array(header.buffer))) {
            return false;
        }
        return this.client.write(data);
    }
    IsConnected() {
        return this.connected;
    }

    async Call(inData) {
        var This = this;
        var retData: any = null;
        try {
            retData = await new Promise((resolve, reject) => {
                This.onData = resolve;
                This.onEnd = reject;
                This.Send(inData);
            });
        } catch (e: any) {
            L(e.message);
        }
        //const header = new Uint32Array(retData);
        //var len = header[0];
        var outData = null;
        if (retData !== null) {
            outData = retData.toString('utf8', 4);
        }
        return outData;
    }
}

export function activate(context: vscode.ExtensionContext) {
    activateXLangDebug(context);
}

export function deactivate() {
	// nothing to do
}
