<template>
    <v-container fluid>
        <v-row>
            <v-col cols="12"
                   md="12">
                <v-textarea clearable
                            clear-icon="mdi-close-circle"
                            label="req"
                            v-model="req"
                            value=""></v-textarea>
            </v-col>
            <v-btn block @click="request">
                query
            </v-btn>
            <v-col cols="12"
                   md="12">
                <v-textarea clearable
                            clear-icon="mdi-close-circle"
                            label="res"
                            v-model="res"
                            value=""></v-textarea>
            </v-col>
        </v-row>
    </v-container>

</template>
<script>
    import ResizablePage from "../components/resizable-page";
    import Control from "../common/control.js"

    export default {
        name: 'EProcess',
        components: {
            ResizablePage,
            Control
        },
        data: () => ({
            req: '',
            res:'',
            height: 100
        }),
        created() {
            Control.setResponse(this.response)
        },
        destroyed() {
            Control.removeResponse(this.response)
        },
        computed: {
            Control
        },
        methods: {
            response: function (event) {
                this.res    = event.data
                this.onResize()
            },
            request: function () {

                Control.request(JSON.parse(this.req))
            }
        },
        mounted() {
            console.log('mounted()')

        }
    }
</script>
<style>

    #mycontainer {
        //position: absolute;
        //width: 100%;
        height: 100%;
        display: flex;
        flex-direction: column;
        overflow: hidden;
    }

    #outer {
        flex: 1;
    }

    #mytable {
    }

        #mytable table {
            //width: 100%;
        }
</style>