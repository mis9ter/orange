<template>
    <v-app>
        <AppBar v-bind:title='this.title' v-bind:req='this.req' v-on:control="control" />
        <v-main>

        </v-main>
        <v-spacer></v-spacer>
        <v-footer app dark>
            <!-- -->
        </v-footer>
    </v-app>
</template>

<script>
import AppBar from './components/AppBar';

export default {
    name: 'App',

    components: {
    AppBar,
    },
    data: () => ({
        message: '^^',
        counter: 1,
        req: {},
        title:'ORANGE'
    }),
    created: function () {
        console.log('App.vue created')
        if (typeof window.chrome.webview != 'undefined') {
            window.chrome.webview.addEventListener('message',
                event => console.log(event.data))
        }
    },
    destroyed: function () {
        console.log('destroyed')
    },
    methods: {
        control: function (doc) {
            //  여기서 host 로 doc를 보낸다. 
            console.log('---- app control ----')
            console.log(doc)
            if (typeof window.chrome.webview != 'undefined') {
                window.chrome.webview.postMessage(JSON.stringify(doc))
            }
            else {
                this.req = doc
            }
        }
    }
};
</script>
<style>
    #app {
        font-family: Avenir, Helvetica, Arial, sans-serif;
        -webkit-font-smoothing: antialiased;
        -moz-osx-font-smoothing: grayscale;
        text-align: center;
        color: #2c3e50;
        margin-top: 0px;
    }
    html {
        overflow-y: hidden;
    }
</style>
