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
                            <template v-slot:item.ProcName="{item}">
                                <v-expansion-panels flat>
                                    <v-expansion-panel>
                                        <v-expansion-panel-header class="title">
                                            {{item.ProcName}}
                                        </v-expansion-panel-header>
                                        <v-expansion-panel-content>
                                            {{item.ProcPath}}
                                        </v-expansion-panel-content>
                                    </v-expansion-panel>
                                </v-expansion-panels>
                            </template>
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
        <v-dialog v-model="dialog"
                    persistent
                    max-width="290">
            <template v-slot:activator="{ on, attrs }">
                <v-btn color="primary"
                        dark
                        v-bind="attrs"
                        v-on="on">
                    Open Dialog
                </v-btn>
            </template>
            <v-card>
                <v-card-title class="headline">
                    {{title}}
                </v-card-title>
                <v-card-text>{{message}}</v-card-text>
                <v-card-actions>
                    <v-spacer></v-spacer>
                    <v-btn color="green darken-1"
                            text
                            @click="dialog = false">
                        확인
                    </v-btn>
                </v-card-actions>
            </v-card>
        </v-dialog>
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
            dialog: false,
            title: '',
            message: '',

            alert: false,
            alertTitle: '',
            alertMessage:'',
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
            Control.setResponse(this.response)
        },
        destroyed() {
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
                this.height = this.getHeight()
            },
            response: function (event) {
                const res = JSON.parse(event.data)
                try {
                    if (true == res.result.ret) {
                        switch (res.header.reqid) {
                            case 0:
                                this.items = []
                                for (var i = 0; i < res.data.row.length; i++) {
                                    //console.log(res.data.row[i])
                                    this.items.push(res.data.row[i])
                                }
                                break;
                        }

                    }
                    else {
                        this.dialog = true;
                        this.title = '오류'
                        this.message    = res.result.msg
                    }
                } catch (error) {
                    this.items  = []
                    console.error(error)
                }
                this.onResize()
                this.loading    = false
            },
            request: async function (obj) {
                this.loading = true
                /*                this.$dialog.notify.info('Test notification', {
                    position: 'top-right',
                    timeout: 5000
                })
                */
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
            this.headers.push({ text: "프로세스", value: "ProcName" })
            //this.headers.push({ text: "파일경로", value: "ProcPath" })
            this.headers.push({ text: "실행횟수", value: "cnt" })
            this.headers.push({ text: "크래시", value: "" })

            //  file
            this.headers.push({ text: "폴더생성", value: "" })
            this.headers.push({ text: "폴더삭제", value: "" })
            this.headers.push({ text: "파일생성", value: "" })
            this.headers.push({ text: "파일삭제", value: "" })
            this.headers.push({ text: "쓰기", value: "" })
            this.headers.push({ text: "읽기", value: "" })            

            //  registry
            this.headers.push({ text: "키생성", value: "" })
            this.headers.push({ text: "키삭제", value: "" })
            this.headers.push({ text: "값생성", value: "" })
            this.headers.push({ text: "값삭제", value: "" })
            this.headers.push({ text: "쓰기", value: "" })
            this.headers.push({ text: "읽기", value: "" })            

            //  networking
            this.headers.push({ text: "DNS쿼리", value: "" })
            this.headers.push({ text: "접속IP", value: "" })
            this.headers.push({ text: "송신", value: "" })
            this.headers.push({ text: "수신", value: "" })
            
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
    .v-expansion-panel {
        box-shadow: none;
        width:30%
    }
</style>