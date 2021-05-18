import Vue from 'vue'
import vuetify from './plugins/vuetify'
import App from './App.vue'
import router from './router'
import VueColumnsResizable from './plugins/vue-columns-resizable'

Vue.use(VueColumnsResizable)
Vue.config.productionTip = false

new Vue({
    vuetify,
    router,
    render: h => h(App)
}).$mount('#app')
