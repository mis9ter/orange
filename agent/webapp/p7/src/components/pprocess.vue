<template>
    <v-container fluid id="mycontainer">
        <v-card>
            <v-card-title>
                PPROCESS
                <v-spacer></v-spacer>
                <v-text-field v-model="keyword"
                              append-icon="mdi-magnify"
                              label="Search"
                              single-line
                              hide-details></v-text-field>
            </v-card-title>
            <div id="outer">
                <resizable-page>
                    <template v-slot:table="tableProps">
                        <v-data-table id="mytable"
                                      :headers="headers"
                                      :items="items"
                                      :item-key="key"
                                      :height="tableProps.tableHeight"
                                      :loading="loading"
                                      :search="keyword"
                                      disable-pagination
                                      fixed-header
                                      hide-default-footer
                                      v-columns-resizable
                                      class="elevation-1">
                            <template v-slot:item.cnt="{ item }">
                                <v-chip :color="getColor(item.cnt)"
                                        dark>
                                    {{ item.cnt }}
                                </v-chip>
                            </template>
                            <v-progress-circular :size="70"
                                                 :width="7"
                                                 color="orange"
                                                 indeterminate>
                            </v-progress-circular>
                        </v-data-table>
                    </template>
                </resizable-page>
            </div>
        </v-card>
    </v-container>

</template>
<script>
    import ResizablePage from "../components/resizable-page";
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
            height: 100,
            loading: false,
            keyword:'',
            req: {
                "header": {
                    "command": "sqlite3.query",
                    "type": "unknown",
                    "query": "select ProcName, ProcPath, count(*) cnt, sum(KernelTime+UserTime) time, sum(KernelTime) ktime, sum(UserTime) utime from process where bootid=? group by ProcPath order by time desc",
                    "bind": [
                        {
                            "type": 0,
                            "value": 50
                        }
                    ],
                    "reqid":0
                }
            }
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
                const res = JSON.parse(event.data)
                switch (res.header.reqid) {
                    case 0:
                        this.items  = []
                        for (var i = 0; i < res.data.row.length; i++) {
                            //console.log(res.data.row[i])
                            this.items.push(res.data.row[i])
                        }
                        break;
                }
                this.onResize()
                this.loading    = false
            },
            request: function (obj) {
                this.loading = true
                Control.request(obj)
            },
            getColor(val) {
                if (val > 400) return 'red'
                else if (val > 200) return 'orange'
                else return 'green'
            },
        },
        mounted() {
            console.log('mounted()')
            this.headers.push({ text: "파일명", value: "ProcName" })
            this.headers.push({ text: "파일경로", value: "ProcPath" })
            this.headers.push({ text: "실행횟수", value: "cnt" })
            this.headers.push({ text: "CPU시간", value: "time" })
            this.headers.push({ text: "커널시간", value: "ktime" })
            this.headers.push({ text: "유저시간", value: "utime" })
            this.request(this.req)
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