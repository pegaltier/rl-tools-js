const start = document.querySelector('#start');
const stop = document.querySelector('#stop');
const seed = document.querySelector('#seed');
const status = document.querySelector('#status');
const pendulum = document.querySelector('#pendulum');
const chart = document.querySelector('#chart');
let worker;

function draw(state, evaluations) {
  const ctx = pendulum.getContext('2d');
  ctx.clearRect(0, 0, pendulum.width, pendulum.height);
  if (state) {
    ctx.beginPath();
    ctx.moveTo(150, 150);
    ctx.lineTo(150 + 110 * Math.sin(state.theta), 150 - 110 * Math.cos(state.theta));
    ctx.lineWidth = 8;
    ctx.stroke();
  }
  const graph = chart.getContext('2d');
  graph.clearRect(0, 0, chart.width, chart.height);
  if (!evaluations.length) return;
  const values = evaluations.map(({ meanReturn }) => meanReturn);
  const min = Math.min(...values);
  const range = Math.max(...values) - min || 1;
  graph.beginPath();
  values.forEach((value, i) => {
    const x = 20 + i * (chart.width - 40) / Math.max(1, values.length - 1);
    const y = chart.height - 20 - (value - min) / range * (chart.height - 40);
    if (i === 0) graph.moveTo(x, y); else graph.lineTo(x, y);
  });
  graph.stroke();
  graph.fillText(`mean return: ${values.at(-1).toFixed(1)}`, 20, 15);
}

function end(message) {
  worker?.terminate();
  worker = null;
  status.textContent = message;
  start.disabled = false;
  stop.disabled = true;
}

start.addEventListener('click', () => {
  const value = Number(seed.value);
  if (seed.value === '' || !Number.isInteger(value) || value < 0 || value > 0x7fffffff) {
    status.textContent = 'Seed must be an integer between 0 and 2147483647';
    return;
  }
  start.disabled = true;
  stop.disabled = false;
  status.textContent = 'Loading...';
  worker = new Worker(new URL('./worker.js', import.meta.url), { type: 'module' });
  worker.onmessage = ({ data }) => {
    if (data.error) { end(`Error: ${data.error}`); return; }
    draw(data.state, data.evaluations);
    status.textContent = `Step ${data.step} / ${data.limit}${data.finished ? ' (finished)' : ''}`;
    if (data.finished) end(status.textContent);
  };
  worker.onerror = event => end(`Error: ${event.message}`);
  worker.postMessage({ seed: value });
});
stop.addEventListener('click', () => end('Stopped'));
