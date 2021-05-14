<template>
	<div class="appbar">
		<v-card height=40>
			<v-app-bar dense dark>
				<v-app-bar-nav-icon></v-app-bar-nav-icon>
				<v-toolbar-title>{{title}}</v-toolbar-title>
				<v-spacer></v-spacer>
				<v-btn icon v-on:click="control('window',0)">
					<v-icon>mdi-minus</v-icon>
				</v-btn>
				<v-btn icon v-on:click="control('window',1)">
					<v-icon>mdi-checkbox-blank-outline</v-icon>
				</v-btn>
				<v-btn icon v-on:click="control('window',2)">
					<v-icon>mdi-close</v-icon>
				</v-btn>
			</v-app-bar>
		</v-card>
	</div>
</template>			
<script>
	export default {
		name: 'AppBar',
		props: {
			title: String,
			req:	Object
		},
        data: () => ({
            req: {},
            title: ''
        }),
		methods: {
            control: function (cmd, type) {
				var doc = { 'command': cmd, 'type': type, 'from': 'appbar', 'to': 'main' }
				this.$emit('control', doc)                
            }
		},
		watch: {
			req: function (value) {
				console.log('---- appbar watch ----')
				console.log(JSON.stringify(value))
            }
        }
	}
</script>