const start = document.querySelector('#start');
const stop = document.querySelector('#stop');
const seed = document.querySelector('#seed');
const iterations = document.querySelector('#iterations');
const status = document.querySelector('#status');
let worker;

function finish(message) {
  worker?.terminate();
  worker = null;
  status.textContent = message;
  start.disabled = false;
  stop.disabled = true;
}

function render(result) {
  const { trajectory } = result;
  document.querySelector('#metric-iterations').textContent = result.iteration;
  document.querySelector('#metric-return').textContent = `${(result.totalReturn * 100).toFixed(2)}%`;
  document.querySelector('#metric-equity').textContent = trajectory.length ? trajectory.at(-1).equity.toFixed(4) : '—';
  document.querySelector('#result').textContent = JSON.stringify(result, null, 2);
  const body = document.querySelector('#points');
  body.replaceChildren();
  trajectory.slice(-12).forEach((point, index) => {
    const row = body.insertRow();
    for (const value of [trajectory.length - Math.min(12, trajectory.length) + index, point.price.toFixed(2), point.equity.toFixed(4), point.position]) {
      row.insertCell().textContent = value;
    }
  });
  const canvas = document.querySelector('#chart');
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  for (const [key, color] of [['price', '#2563a6'], ['equity', '#14835b']]) {
    if (!trajectory.length) continue;
    const values = trajectory.map(point => point[key]);
    const min = Math.min(...values);
    const max = Math.max(...values);
    const range = max - min || 1;
    const digits = key === 'price' ? 2 : 4;
    document.querySelector(`#${key}-range`).textContent = `● ${key === 'price' ? 'Price' : 'Equity'}: ${min.toFixed(digits)}–${max.toFixed(digits)}`;
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    values.forEach((value, i) => {
      const x = 20 + i * (canvas.width - 40) / Math.max(1, values.length - 1);
      const y = canvas.height - 20 - (value - min) / range * (canvas.height - 40);
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }
}

start.addEventListener('click', () => {
  const s = Number(seed.value);
  const n = Number(iterations.value);
  if (!seed.value || !Number.isInteger(s) || s < 0 || s > 0x7fffffff) {
    status.textContent = 'Seed must be an integer between 0 and 2147483647.';
    return;
  }
  if (!iterations.value || !Number.isSafeInteger(n) || n < 0) {
    status.textContent = 'Iterations must be a non-negative integer.';
    return;
  }
  start.disabled = true;
  stop.disabled = false;
  status.textContent = 'Loading trainer…';
  worker = new Worker(new URL('./worker.js', import.meta.url), { type: 'module' });
  worker.onmessage = ({ data }) => {
    if (data.error) finish(`Error: ${data.error}`);
    else if (data.result) {
      render(data.result);
      finish(`Evaluation complete after ${data.result.iteration} iterations.`);
    } else status.textContent = `Training: iteration ${data.progress} / ${Math.min(n, data.limit)}`;
  };
  worker.onerror = event => finish(`Error: ${event.message}`);
  worker.postMessage({ seed: s, iterations: n });
});
stop.addEventListener('click', () => finish('Stopped. The training state was discarded.'));
