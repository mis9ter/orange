<template>
    <v-container fluid id="mycontainer">
        <h2>EPROCESS</h2>
        <div id="outer">
            <v-data-table id="mytable"
                          :headers="headers"
                          :items="items"
                          :height="height"
                          disable-pagination
                          fixed-header
                          hide-default-footer
                          class="elevation-1">
            </v-data-table>
        </div>
    </v-container>

</template>
<script>
    import ResizablePage from "../components/resizable-page";

    export default {
        name: 'PProcess',
        data: () => ({
            headers: [
                { text: "Header A", value: "a" },
                { text: "Header B, longer", value: "b" },
                { text: "Header C", value: "c" }
            ],
            items: [],
            height: 100
        }),
        computed: {

        },
        methods: {
            getHeight() {
                var h   = window.innerHeight -
                            this.$refs.resizableDiv.getBoundingClientRect().y - 59;
                return h;

            },
            onResize() {
                console.log('onResize()')
                this.height = this.getHeight()
            }
        },
        mounted() {
            console.log('mounted()')
            for (var i = 1; i <= 100; i++) {
                this.items.push({ a: "Row " + i, b: "Column B", c: "Column C", key:i });
            }
            this.height = this.getHeight();
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