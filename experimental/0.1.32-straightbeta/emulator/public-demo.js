import './slice-shaped-ui.js';
// SPDX-License-Identifier: GPL-3.0-only
// Exercise the same File inputs/parser used for the user's own imported pair.
const demo = document.getElementById('slice-demo');
demo.textContent = 'Load public demo Â· 120 BPM';
demo.onclick = async () => {
  demo.disabled = true;
  const status = document.getElementById('slice-status');
  try {
    const names = ['acultur8-demo_slices.wav', 'acultur8-demo_markers.json'];
    const files = await Promise.all(names.map(async name => {
      const response = await fetch(new URL('../../../demo/' + name, import.meta.url));
      if (!response.ok) throw Error('Demo download failed (' + response.status + ').');
      return new File([await response.blob()], name);
    }));
    ['slice-audio', 'slice-markers'].forEach((id, index) => {
      const transfer = new DataTransfer();
      transfer.items.add(files[index]);
      document.getElementById(id).files = transfer.files;
    });
    document.getElementById('slice-import-button').click();
  } catch (error) {
    status.textContent = error.message + ' You can download the WAV and JSON from the home page and choose them manually.';
    status.classList.add('slice-error');
  } finally { demo.disabled = false; }
};
