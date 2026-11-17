




export class Client{
    constructor(){
        this.setParametersCallback = null;
        this.worker = new Worker(new URL('./training_worker.js', import.meta.url), {type: "module"});
        this.worker.addEventListener("error", (error) => {
            console.error("An error occurred in the Web Worker:", error);
            const label = document.getElementById('car-loadingLabel');
            label.textContent = `Worker error: ${error.message}`;
            label.style.display = 'block';
        });

        this.worker.addEventListener("message", (event) => {
            this.onMessage(event.data);
        });
    }
    setEnvironmentCallbacks({setParametersCallback, setStateCallback, setActionCallback, setTruncatedCallback}){
        this.setParametersCallback = setParametersCallback;
        this.setStateCallback = setStateCallback;
        this.setActionCallback = setActionCallback;
        this.setTruncatedCallback = setTruncatedCallback
    }

    onMessage(message){
        let {channel, data} = message
        if(channel === "setParameters"){
            if(this.setParametersCallback){
                this.setParametersCallback(data)
            }
        }
        else{
            if(channel === "setState"){
                if(this.setStateCallback){
                    this.setStateCallback(data)
                }
            }
            else{
                if(channel === "setAction"){
                    if(this.setActionCallback){
                        this.setActionCallback(data)
                    }
                }
                else{
                    if(channel === "setTruncated"){
                        if(this.setTruncatedCallback){
                            this.setTruncatedCallback(data)
                        }
                    }
                }
            }
        }
    }
    sendMessage(channel, data){
        this.worker.postMessage({channel, data});
    }
}
