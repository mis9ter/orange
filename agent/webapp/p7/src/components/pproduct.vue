<template>
    <v-container fluid id="mycontainer">
        <h2>PPRODUCT</h2>
        <div id="outer">
            <resizable-page>
                <template v-slot:table="tableProps">
                    <v-data-table id="mytable"
                                  :headers="headers"
                                  :items="items"
                                  :item-key="key"
                                  :height="tableProps.tableHeight"
                                  disable-pagination
                                  fixed-header
                                  hide-default-footer
                                  class="elevation-1">
                    </v-data-table>
                </template>
            </resizable-page>
        </div>
    </v-container>

</template>
<script>
    import ResizablePage    from "../components/resizable-page";
    import Control from "../common/control.js"

    export default {
        name: 'PProcess',
        components: {
            ResizablePage,
            Control
        },
        data: () => ({
            headers: [],
            items: [],
            height: 100
        }),
        computed: {

        },
        created() {
            console.log('---- pproduct created ----')
            Control.test()
            Control.setResponse(this.response)
        },
        destroyed() {
            console.log('---- pproduct destroyed ----')
            Control.removeResponse(this.response)
        },
        methods: {
            getHeight() {
                //console.log(document.getElementById("mytable"));
                var h = document.getElementById("app-vmain").clientHeight;
                //console.log('getHeight()=' + h)
                return h;

            },
            onResize() {
                console.log('onResize()')
                this.height = this.getHeight()
            },
            response: function (event) {
                console.log('---- pproduct response ----')
                //console.log(event.data)
                const doc = JSON.parse(event.data)
                console.log(doc.header.command)
                if ('pproduct.header' == doc.header.command) {
                    this.headers = []
                    for (var i = 0; i < doc.data.length; i++) {
                        this.headers.push(doc.data[i])
                    }
                    this.request('pproduct.data', 0)
                }
                else if ('pproduct.data' == doc.header.command) {
                    this.items = []
                    for (var i = 0; i < doc.data.length; i++) {
                        console.log(doc.data[i])
                        this.items.push(doc.data[i])
                    }
                }
                this.onResize()
            },
            request: function (cmd, type) {
                console.log('---- pproduct request ----')
                var obj = new Object()

                obj.header  = new Object()
                obj.header.command  = cmd
                obj.header.type = type
                obj.header.from = 'pproduct'
                obj.header.to = 'main'

                //var doc = { 'command': cmd, 'type': type, 'from': 'pproduct', 'to': 'main' }
                //this.$emit('request', doc)
                var doc = JSON.stringify(obj)
                Control.request(obj)
            }
        },
        mounted() {
            console.log('mounted()')
            this.request('pproduct.header', 0) 
            //this.headers.push({ text: "Header A", value: "a" })
            //this.headers.push({ text: "Header B, longer", value: "b" })
            //this.headers.push({ text: "Header C", value: "c" })

            //for (var i = 1; i <= 100; i++) {
            //    this.items.push({ key: i, a: "Row " + i, b: "Column B", c: "Column C" });
            //    //this.items.push({ i: { a: "Row " + i, b: "Column B", c: "Column C" } });
            //}
            window.addEventListener("resize", this.onResize);
            this.$nextTick(this.onResize);
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