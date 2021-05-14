import Vue from 'vue'
import VueRouter from 'vue-router'

import PProcess from '../components/pprocess.vue'
import PProduct from '../components/pproduct.vue'
import EProcess from '../components/eprocess.vue'
import EProduct from '../components/eproduct.vue'
import Foo from '../components/Foo.vue'
import Bar from '../components/Bar.vue'
import NotFound from '../components/NotFound.vue'

Vue.use(VueRouter)

export default new VueRouter({
    routes: [
        { name: 'foo', path: '/foo', component: Foo },
        { name: 'bar', path: '/bar', component: Bar },
        { name: 'pprocess', path: '/pprocess', component: PProcess },
        { name: 'pproduct', path: '/pproduct', component: PProduct },
        { name: 'eprocess', path: '/eprocess', component: EProcess },
        { name: 'eproduct', path: '/eproduct', component: EProduct },
        { name: 'a1', path: '/a1', component: Foo },
        { name: 'a2', path: '/a2', component: Bar },
        { name: 'a3', path: '/a3', component: Foo },
        { name: 'NotFound', path: '*', component: NotFound }
    ],
    components: {
        PProduct
    },
    methods: {
        request: function (doc) {
            //  여기서 host 로 doc를 보낸다. 
            console.log('---- router request ----')
            console.log(doc)
            if (typeof window.chrome.webview != 'undefined') {
                window.chrome.webview.postMessage(JSON.stringify(doc))
            }
            else {
                console.log('---- webview is not found')
                this.req = doc
            }
        }
    }
})