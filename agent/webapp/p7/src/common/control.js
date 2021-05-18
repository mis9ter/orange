
export default {
    data: () => ({
        responseCallback: 'undefined',
    }),

    created() {      

    },
    test: function () {
        console.log('---- test ----')

    },
    setResponse: function (callback) {
        console.log('---- setResponse ----')
        if (typeof window.chrome.webview != 'undefined') {
            window.chrome.webview.addEventListener('message', callback)
        }
    },
    removeResponse: function (callback) {
        console.log('---- removeResponse ----')
        if (typeof window.chrome.webview != 'undefined') {
            window.chrome.webview.removeEventListener('message', callback)
        }
    },
    request: function (doc) {
        //  여기서 host 로 doc를 보낸다. 
        //console.log('---- control request ----')
        //console.log(doc)
        if (typeof window.chrome.webview != 'undefined') {
            window.chrome.webview.postMessage(JSON.stringify(doc))
        }
        else {
            console.log('---- webview is not found')
            this.req = doc
        }
    },
    response: function (data) {
        console.log('---- control response ----')
        console.log(data)
    }
}
