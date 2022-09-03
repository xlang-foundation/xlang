'use strict';
import * as vscode from 'vscode';
import { activateXLangDebug} from './activateXLangDebug';

let devSrvStatusBarItem: vscode.StatusBarItem;

function QueryDevOpsNodes(cb){
    //if(xlangRuntime) {
    //    xlangRuntime.Call("$cmd:enumNodes",cb);
    //}
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

  function webviewTest2()
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
    //webviewTest();
	// create a new status bar item that we can now manage
	devSrvStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
	devSrvStatusBarItem.command = devSrvCmdId;
	subscriptions.push(devSrvStatusBarItem);
    
    devSrvStatusBarItem.text = `X-DevServer`;
	devSrvStatusBarItem.show();
    activateXLangDebug(context);
}

export function deactivate() {
	// nothing to do
}
