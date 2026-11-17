import createCarModule from '../../dist/car-browser.js';

const pending = [];
let playbackSpeed = 3;
self.onmessage = ({ data }) => {
  if (data.channel === 'setPlaybackSpeed') playbackSpeed = data.data;
  else pending.push(data);
};

async function run() {
  const module = await createCarModule();
  const state = module._proxy_create(1);
  if (!state) throw new Error('Failed to allocate car training state');
  try {
    while (true) {
      while (pending.length) {
        const message = JSON.stringify(pending.shift());
        const length = module.lengthBytesUTF8(message) + 1;
        const pointer = module._malloc(length);
        if (!pointer) throw new Error('Failed to allocate car message');
        try {
          module.stringToUTF8(message, pointer, length);
          module._proxy_step_message(state, pointer);
        } finally {
          module._free(pointer);
        }
      }
      if (module._proxy_num_messages(state) === 0) module._proxy_step(state);
      if (module._proxy_num_messages(state) > 0) {
        const pointer = module._proxy_pop_message(state);
        try {
          self.postMessage(JSON.parse(module.UTF8ToString(pointer)));
        } finally {
          module._proxy_delete_message(pointer);
        }
      }
      await new Promise(resolve => setTimeout(resolve, 10 / playbackSpeed));
    }
  } finally {
    module._proxy_destroy(state);
  }
}

run().catch(error => {
  setTimeout(() => { throw error; });
});
