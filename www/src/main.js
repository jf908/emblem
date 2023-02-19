import init, { build } from '../../pkg';
import * as monaco from 'monaco-editor/esm/vs/editor/editor.api';
import 'github-markdown-css';

let loaded = false;
init().then(() => {
  loaded = true;
});

let output = document.querySelector('#output');
let debug = document.querySelector('#debug');
function onChange(value) {
  if (loaded) {
    let out;
    try {
      out = build(value);
    } catch (ex) {
      console.error(ex);

      const match = ex.match(/found at (\d+):(\d+):(\d+):(\d+)/);
      if (match) {
        monaco.editor.setModelMarkers(editor.getModel(), 'emblem', [
          {
            startLineNumber: parseInt(match[1]),
            startColumn: parseInt(match[2]),
            endLineNumber: parseInt(match[3]),
            endColumn: parseInt(match[4]),
            message: ex,
          },
        ]);
      } else {
        monaco.editor.setModelMarkers(editor.getModel(), 'emblem', []);
      }

      return;
    }
    monaco.editor.setModelMarkers(editor.getModel(), 'emblem', []);
    const [result, d] = out;
    output.innerHTML = result;
    debug.textContent = d;
  }
}

const editor = monaco.editor.create(document.querySelector('.textarea'), {
  automaticLayout: true,
  theme:
    window.matchMedia &&
    window.matchMedia('(prefers-color-scheme: dark)').matches
      ? 'vs-dark'
      : 'vs',
});

window
  .matchMedia('(prefers-color-scheme: dark)')
  .addEventListener('change', (event) => {
    monaco.editor.setTheme(event.matches ? 'vs-dark' : 'vs');
  });

editor.getModel().onDidChangeContent(() => {
  onChange(editor.getValue());
});
