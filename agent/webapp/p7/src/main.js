import Vue from 'vue'
import vuetify from './plugins/vuetify'
import App from './App.vue'
import router from './router'
import VueColumnsResizable from './plugins/vue-columns-resizable'
//import VuetifyDialog from 'vuetify-dialog'

Vue.use(VueColumnsResizable)
//Vue.use(VuetifyDialog)

Vue.config.productionTip = false

new Vue({
    vuetify,
    router,
    render: h => h(App)
}).$mount('#app')
