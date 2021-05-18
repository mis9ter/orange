<template>
    <v-app>
        <v-app-bar app clipped-left>
            <v-app-bar-nav-icon @click="onDrawer()"></v-app-bar-nav-icon>
            <v-toolbar-title>{{title}}</v-toolbar-title>
            <v-spacer></v-spacer>
            <v-btn-toggle v-model="mode"
                            mandatory
                            dark dense
                            @change="onMode">
                <v-btn v-for="t in modes" :value="t.value">
                    {{t.title}}
                </v-btn>
            </v-btn-toggle>
            <v-spacer></v-spacer>
            <v-btn icon>
                <v-icon>mdi-magnify</v-icon>
            </v-btn>

        </v-app-bar>
        <v-card>
            <v-navigation-drawer app
                                    clipped
                                    permanent
                                    id="drawer"
                                    v-model="drawer"
                                    :mini-variant="!drawer">
                <template v-slot:prepend>
                    <v-list-item two-line>
                        <v-list-item-avatar>
                            <img src="https://randomuser.me/api/portraits/women/81.jpg">
                        </v-list-item-avatar>

                        <v-list-item-content>
                            <v-list-item-title>{{modes[mode].display}}</v-list-item-title>
                            <v-list-item-subtitle>{{modes[mode].desc}}</v-list-item-subtitle>
                        </v-list-item-content>
                    </v-list-item>
                </template>

                <v-divider></v-divider>

                <v-list dense>
                    <v-list-item v-for="t in items[mode]"
                                    @click="onType(t.value, t.path)"
                                    :value="t.value">
                        <v-list-item-icon>
                            <v-icon>{{ t.icon }}</v-icon>
                        </v-list-item-icon>
                        <v-list-item-content class="text-left">
                            <v-list-item-title>{{ t.title }}</v-list-item-title>
                        </v-list-item-content>
                    </v-list-item>
                </v-list>
            </v-navigation-drawer>
        </v-card>

        <v-main id="app-vmain">
            <!-- Provides the application the proper gutter -->
            <v-container fluid>
                <!-- If using vue-router -->
                <router-view @request="request"></router-view>
            </v-container>
        </v-main>
        <v-footer app>
            <!-- -->
            {{path}}
        </v-footer>
    </v-app>
</template>

<script>
    import AppBar from './components/AppBar'

    //const { isNavigationFailure, NavigationFailureType } = VueRouter

    export default {
        data: () => ({
            title: "ORANGE",
            mode: 0,
            type: 0,
            drawer: true,
            path:window.location.pathname,
            links: [
                'Dashboard',
                '성능',
                '장애',
                '',
            ],
            modes: [
                { value: 0, title: '성능', display:'성능측정', desc:'' },
                { value: 1, title: '장애', display:'장애측정', desc:'' },
                { value: 2, title: '광고', display:'', desc:'' },
            ],
            items: [
                [
                    { value:0, title: '프로세스', icon: 'mdi-home-city', path: '/pprocess' },
                    { value:1, title: '설치제품', icon: 'mdi-account', path: '/pproduct' },
                ],
                [
                    { value:0, title: '프로세스', icon: 'mdi-home-city', path: '/eprocess' },
                    { value:1, title: '설치제품', icon: 'mdi-account', path: '/eproduct' },
                ],
                [
                    { value:0, title: 'Home', icon: 'mdi-home-city', path: '/a1' },
                    { value:1, title: 'My Account', icon: 'mdi-account', path: '/a2' },
                    { value:2, title: 'Users', icon: 'mdi-account-group-outline', path: '/a3' },
                ],
            ],
            req: []
        }),
        created() {
            this.$router.options.routes.forEach(route => {
                //console.log(route.name + ':' + route.path)
            })
            this.path = this.$router.currentRoute.path
            console.log('App.vue created')
        },
        computed: {
            mini() {
                return false;
                return this.$vuetify.breakpoint.mdAndDown;
                switch (this.$vuetify.breakpoint.name) {
                    case 'xs': return true
                    case 'sm': return true
                    case 'md': return false
                    case 'lg': return false
                    case 'xl': return false
                }
            }
        },
        methods: {
            showToast(msg) {
                this.$toasted.show(msg)
            },
            onDrawer() {
                var obj = document.querySelector('#drawer')
                //console.log(obj)
                this.drawer = !this.drawer;
            },
            onMode() {
                console.log('mode:' + this.mode)
            },
            onType(t, path) {
                this.type = t
                console.log('mode:' + this.mode + " type:" + this.type + ' :' + path)
                if (this.$router.currentRoute.path === path) {
                    console.log('---- same path')
                }
                else {
                    console.log('---- another path')
                    this.$router.push(path).catch(failure => {
                        console.log(failure)
                    })
                }
                this.path   = this.$router.currentRoute.path
            },
            request: function (doc) {
                //  여기서 host 로 doc를 보낸다. 
                console.log('---- app request ----')
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
    }
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
