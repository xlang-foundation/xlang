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
import { WorkspaceFolder, DebugConfiguration, ProviderResult, CancellationToken } from 'vscode';
import { XLangDebugSession } from './xLangDebug';

async function SetExePath() : Promise<string | undefined>
{
	let exePath : any;
	if (process.platform === "win32") {
		const ret = await vscode.window.showOpenDialog({
			openLabel:'choose',
			canSelectFiles: true,
			canSelectMany: false,
			filters:{'xlang executable file': ['exe']},
			title: 'Select XLang Executable File'
		});
		if (ret?.length > 0)
			exePath = ret[0].fsPath;
	} else if (process.platform === "linux" || process.platform === "darwin") {
		const ret = await vscode.window.showOpenDialog({
			openLabel:'choose',
			canSelectFiles: true,
			canSelectMany: false,
			filters:{'xlang executable file': ['*']},
			title: 'Select XLang Executable File'
		});
		if (ret?.length > 0)
			exePath = ret[0].fsPath;
	}
	if (exePath)
	{
		await vscode.workspace.getConfiguration('XLangDebugger').update('ExePath', exePath, vscode.ConfigurationTarget.Global);//save to global config
	}
	//exePath = vscode.workspace.getConfiguration('XLangDebugger').get<string>('ExePath');
	return exePath;
};

export async function activateXLangDebug(context: vscode.ExtensionContext, factory?: vscode.DebugAdapterDescriptorFactory) {
	// check and set the xlang executable file
	let exePath = vscode.workspace.getConfiguration('XLangDebugger').get<string>('ExePath');
	if (!exePath || exePath === '')
	{
		exePath = await SetExePath();
	}

	context.subscriptions.push(
		vscode.commands.registerCommand('extension.xlang.runEditorContents', (resource: vscode.Uri) => {
			let targetResource = resource;
			if (!targetResource && vscode.window.activeTextEditor) {
				targetResource = vscode.window.activeTextEditor.document.uri;
			}
			if (targetResource) {
				vscode.debug.startDebugging(undefined, {
					type: 'xlang',
					name: 'Run File',
					request: 'launch',
					program: targetResource.fsPath
				},
					{ noDebug: true }
				);
			}
		}),
		vscode.commands.registerCommand('extension.xlang.debugEditorContents', (resource: vscode.Uri) => {
			let targetResource = resource;
			if (!targetResource && vscode.window.activeTextEditor) {
				targetResource = vscode.window.activeTextEditor.document.uri;
			}
			if (targetResource) {
				vscode.debug.startDebugging(undefined, {
					type: 'xlang',
					name: 'Debug File',
					request: 'launch',
					program: targetResource.fsPath,
					stopOnEntry: true
				});
			}
		}),
		vscode.commands.registerCommand('extension.xlang.toggleFormatting', (variable) => {
			const ds = vscode.debug.activeDebugSession;
			if (ds) {
				ds.customRequest('toggleFormatting');
			}
		})
	);

	context.subscriptions.push(vscode.commands.registerCommand('extension.xlang.getProgramName', config => {
		return vscode.window.showInputBox({
			placeHolder: "Please enter the name of a xlang file in the workspace folder",
			value: "test.x"
		});
	}));

	// extension command for set xlang executable file
	context.subscriptions.push(vscode.commands.registerCommand('extension.xlang.setExcutablePath', config => {
		return SetExePath();
	}));

	// register a configuration provider for 'xlang' debug type
	const provider = new XLangConfigurationProvider();
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('xlang', provider));

	// register a dynamic configuration provider for 'xlang' debug type
	context.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('xlang', {
		provideDebugConfigurations(folder: WorkspaceFolder | undefined): ProviderResult<DebugConfiguration[]> {
			return [
				{
					name: "Launch",
					request: "launch",
					type: "xlang",
					program: "${file}"
				},
				{
					name: "Attach",
					request: "launch",
					type: "xlang",
					program: "${file}"
				}
			];
		}
	}, vscode.DebugConfigurationProviderTriggerKind.Dynamic));

	if (!factory) {
		factory = new InlineDebugAdapterFactory();
	}
	context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('xlang', factory));
	if ('dispose' in factory) {
		context.subscriptions.push(factory);
	}

	// override VS Code's default implementation of the debug hover
	// here we match only XLang "variables", that are words starting with an '$'
	context.subscriptions.push(vscode.languages.registerEvaluatableExpressionProvider('xlang', {
		provideEvaluatableExpression(document: vscode.TextDocument, position: vscode.Position): vscode.ProviderResult<vscode.EvaluatableExpression> {

			const VARIABLE_REGEXP = /\$[a-z][a-z0-9]*/ig;
			const line = document.lineAt(position.line).text;

			let m: RegExpExecArray | null;
			while (m = VARIABLE_REGEXP.exec(line)) {
				const varRange = new vscode.Range(position.line, m.index, position.line, m.index + m[0].length);

				if (varRange.contains(position)) {
					return new vscode.EvaluatableExpression(varRange);
				}
			}
			return undefined;
		}
	}));

	// override VS Code's default implementation of the "FORCE_INLINE values" feature"
	context.subscriptions.push(vscode.languages.registerInlineValuesProvider('xlang', {

		provideInlineValues(document: vscode.TextDocument, viewport: vscode.Range, context: vscode.InlineValueContext) : vscode.ProviderResult<vscode.InlineValue[]> {

			const allValues: vscode.InlineValue[] = [];

			for (let l = viewport.start.line; l <= context.stoppedLocation.end.line; l++) {
				const line = document.lineAt(l);
				var regExp = /\$([a-z][a-z0-9]*)/ig;	// variables are words starting with '$'
				do {
					var m = regExp.exec(line.text);
					if (m) {
						const varName = m[1];
						const varRange = new vscode.Range(l, m.index, l, m.index + varName.length);

						// some literal text
						//allValues.push(new vscode.InlineValueText(varRange, `${varName}: ${viewport.start.line}`));

						// value found via variable lookup
						allValues.push(new vscode.InlineValueVariableLookup(varRange, varName, false));

						// value determined via expression evaluation
						//allValues.push(new vscode.InlineValueEvaluatableExpression(varRange, varName));
					}
				} while (m);
			}

			return allValues;
		}
	}));
	return factory;
}

const ipPortRegex = /^((localhost)|((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)):([0-9]|[1-9]\d|[1-9]\d{2}|[1-9]\d{3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5])$/i
const ipRegex = /^((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})\.((2(5[0-5]|[0-4]\d))|[0-1]?\d{1,2})$/;
const ipHostPortRegex = /^(?:(?:\d{1,3}\.){3}\d{1,3}|\[(?:[a-fA-F0-9:]+)\]|(?:[a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+|\w+):\d{1,5}$/;

class XLangConfigurationProvider implements vscode.DebugConfigurationProvider {

	/**
	 * Massage a debug configuration just before a debug session is being launched,
	 * e.g. add all missing attributes to the debug configuration.
	 */
	async resolveDebugConfiguration(folder: WorkspaceFolder | undefined, config: DebugConfiguration, token?: CancellationToken): ProviderResult<DebugConfiguration> {

		// if launch.json is missing or empty
		if (!config.type && !config.request && !config.name) {
			const editor = vscode.window.activeTextEditor;
			if (editor && editor.document.languageId === 'xlang') {
				const items = ['launch', 'attach'];
				const options: vscode.QuickPickOptions = {
					title: 'Select a mode to run debug',
					placeHolder: 'launch'
				};
				let mode = await vscode.window.showQuickPick(items, options);
				if (mode)
				{
					config.request = mode;
				}
				else
				{
					config.request = 'launch';
				}
				if (config.request === 'attach')
				{
					let dbgAddr = await vscode.window.showInputBox({value: "localhost:3142", prompt: "input remote xlang dbg address and port, empty or cancel to run xlang on a random port locally", placeHolder: "ip or host name:port"});
					if (dbgAddr)
					{
						dbgAddr = dbgAddr.replace(/\s/g, '');
						if(ipHostPortRegex.test(dbgAddr))
						{
							const strList = dbgAddr.split(":");
							config.dbgIp = strList[0];
							config.dbgPort = Number(strList[1]);
						}
						else
						{
							await vscode.window.showErrorMessage("please input valid address and port to attach, debugging stopped", { modal: true }, "ok");
							return undefined;
						}
					}
				}
				
				config.type = 'xlang';
				config.name = 'Dynamic Debug';
				config.program = '${file}';
				config.stopOnEntry = true;
			}
		}

		if (!config.program) {
			return vscode.window.showInformationMessage("Cannot find a program to debug").then(_ => {
				return undefined;	// abort launch
			});
		}

		return config;
	}
}

class InlineDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
	private _xLangDebugSession:XLangDebugSession = new XLangDebugSession();
	public getDbgSession(){
		return this._xLangDebugSession;
	}
	createDebugAdapterDescriptor(_session: vscode.DebugSession): ProviderResult<vscode.DebugAdapterDescriptor> {
		return new vscode.DebugAdapterInlineImplementation(this._xLangDebugSession);
	}
}
