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

'use strict';
import * as vscode from 'vscode';
import { activateXLangDebug } from './activateXLangDebug';


export function activate(context: vscode.ExtensionContext) {
    // register a command that is invoked when the status bar
    // item is selected
    // const devSrvCmdId = 'xlang.DevServer.Commands';
    // const subscriptions = context.subscriptions;
    // subscriptions.push(vscode.commands.registerCommand(devSrvCmdId, () => {
    //     QueryDevOpsNodes((retVals)=>{
    //         var nodeList = JSON.parse(retVals);
    //         vscode.window.showQuickPick(nodeList).then(
    //             option => {
    //                 vscode.window.showInformationMessage('Selected:'+option);
    //             }
    //         );
    //         //vscode.window.showInformationMessage(`Yeah, ${n} line(s) selected... Keep going!`);
    //     });
    // }));
    // //webviewTest();
    // // create a new status bar item that we can now manage
    // devSrvStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    // devSrvStatusBarItem.command = devSrvCmdId;
    // subscriptions.push(devSrvStatusBarItem);

    // devSrvStatusBarItem.text = `X-DevServer`;
    // devSrvStatusBarItem.show();
    activateXLangDebug(context);
}

export function deactivate() {
    // nothing to do
}
