'use strict';

import * as Net from 'net'
import * as vscode from 'vscode';
import { activateXLangDebug} from './activateXLangDebug';


export function activate(context: vscode.ExtensionContext) {
	activateXLangDebug(context);
}

export function deactivate() {
	// nothing to do
}

