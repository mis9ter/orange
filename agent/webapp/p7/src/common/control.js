
export default {
    created() {      
        console.log('common/control created')
        if (typeof window.chrome.webview != 'undefined') {
            window.chrome.webview.addEventListener('message',
                event => this.response(event.data))
        }
    },
    request: function (doc) {
        //  ���⼭ host �� doc�� ������. 
        console.log('---- request ----')
        console.log(doc)
        if (typeof window.chrome.webview != 'undefined') {
            window.chrome.webview.postMessage(JSON.stringify(doc))
        }
        else {
            console.log('---- webview is not found')
            this.req = doc
        }
    },
    response: function (data) {
        console.log('---- response ----')
        console.log(data)
    }
}
