https://code.visualstudio.com/api/extension-guides/overview

# Linter
https://www.testim.io/blog/what-is-a-linter-heres-a-definition-and-quick-start-guide/

# for case can't build, run line below in vscode termial at x\vscode folder
yarn
# if not run yarn, in windows, run command below in powershell first
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

npm run -S esbuild-base -- --sourcemap --sources-content=false

# for publish
vsce package
vsce publish

then upload
