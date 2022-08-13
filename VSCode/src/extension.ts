'use strict';
import * as Net from 'net';
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
    private callQ: CallInfo[] = [];
    private onNotify: Function = null;
    constructor(cbNotify) {
        this.onNotify = cbNotify;
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
                let tagNoti = "$notify$";
                if (outData.startsWith(tagNoti)){
                    if (This.onNotify) {
                        This.onNotify("notify", outData.substr(tagNoti.length));
                    }
                }
                else
                {
                    let callData = This.callQ.shift();
                    callData?.cb(outData);
                }
                //send next one if have
                This.canSent = true;
                This.Send();
            }).on('end', function () {
                This.callQ = [];
                This.canSent = false;
                This.connected = false;
                if (This.onNotify) {
                    This.onNotify("end","");
                }
            }).on('error', function (error) {
                console.log(error);
                if (This.onNotify) {
                    This.onNotify("error",error);
                }
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


let devSrvStatusBarItem: vscode.StatusBarItem;
let xlangRuntime:XLangRuntime;

function QueryDevOpsNodes(cb){
    if(xlangRuntime) {
        xlangRuntime.Call("$cmd:enumNodes",cb);
    }
}

const cats = {
    'Coding Cat': 'https://media.giphy.com/media/JIX9t2j0ZTN9S/giphy.gif',
    'Compiling Cat': 'https://media.giphy.com/media/mlvseq9yvZhba/giphy.gif'
  };
  function getWebviewContent(cat: keyof typeof cats) {
    return `<!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Cat Coding</title>
  </head>
  <body>
      <img src="${cats[cat]}" width="300" />
  </body>
  </html>`;
  }

  function webviewTest()
  {
    const panel = vscode.window.createWebviewPanel(
        'catCoding',
        'Cat Coding',
        vscode.ViewColumn.Two,
        {
            // Enable scripts in the webview
          enableScripts: true
        }
      );

      let iteration = 0;
      const updateWebview = () => {
        const cat = iteration++ % 2 ? 'Compiling Cat' : 'Coding Cat';
        panel.title = cat;
        panel.webview.html = getWebviewContent(cat);
      };

      // Set initial content
      updateWebview();

      // And schedule updates to the content every second
      setInterval(updateWebview, 1000);
  }
export function activate( context: vscode.ExtensionContext) {
    	// register a command that is invoked when the status bar
	// item is selected
	const devSrvCmdId = 'xlang.DevServer.Commands';
    const subscriptions = context.subscriptions;
	subscriptions.push(vscode.commands.registerCommand(devSrvCmdId, () => {
        QueryDevOpsNodes((retVals)=>{
            var nodeList = JSON.parse(retVals);
            vscode.window.showQuickPick(nodeList).then(
                option => {
                    vscode.window.showInformationMessage('Selected:'+option);
                }
            );
            //vscode.window.showInformationMessage(`Yeah, ${n} line(s) selected... Keep going!`);
        });
	}));
    webviewTest();
	// create a new status bar item that we can now manage
	devSrvStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
	devSrvStatusBarItem.command = devSrvCmdId;
	subscriptions.push(devSrvStatusBarItem);
    
    devSrvStatusBarItem.text = `X-DevServer`;
	devSrvStatusBarItem.show();
    const factory = activateXLangDebug(context);
    if(factory){
        const dbgSession  = factory.getDbgSession();
        if(dbgSession){
            xlangRuntime = dbgSession.getRuntime();
        }
    }
}

export function deactivate() {
	// nothing to do
}
