(function webpackUniversalModuleDefinition(root, factory) {
	if(typeof exports === 'object' && typeof module === 'object')
		module.exports = factory();
	else if(typeof define === 'function' && define.amd)
		define("VueToast", [], factory);
	else if(typeof exports === 'object')
		exports["VueToast"] = factory();
	else
		root["VueToast"] = factory();
})(self, function() {
return /******/ (() => { // webpackBootstrap
/******/ 	"use strict";
/******/ 	var __webpack_modules__ = ({

/***/ 6:
/***/ ((__unused_webpack_module, __webpack_exports__, __webpack_require__) => {


// EXPORTS
__webpack_require__.d(__webpack_exports__, {
  "default": () => /* binding */ src
});

// UNUSED EXPORTS: Component, Positions

;// CONCATENATED MODULE: ./node_modules/vue-loader/lib/loaders/templateLoader.js??vue-loader-options!./node_modules/vue-loader/lib/index.js??vue-loader-options!./src/js/Component.vue?vue&type=template&id=6dd22dbe&
var render = function () {var _vm=this;var _h=_vm.$createElement;var _c=_vm._self._c||_h;return _c('transition',{attrs:{"enter-active-class":_vm.transition.enter,"leave-active-class":_vm.transition.leave}},[_c('div',{directives:[{name:"show",rawName:"v-show",value:(_vm.isActive),expression:"isActive"}],staticClass:"v-toast__item",class:[("v-toast__item--" + _vm.type), ("v-toast__item--" + _vm.position)],attrs:{"role":"alert"},on:{"mouseover":function($event){return _vm.toggleTimer(true)},"mouseleave":function($event){return _vm.toggleTimer(false)},"click":_vm.whenClicked}},[_c('div',{staticClass:"v-toast__icon"}),_vm._v(" "),_c('p',{staticClass:"v-toast__text",domProps:{"innerHTML":_vm._s(_vm.message)}})])])}
var staticRenderFns = []


;// CONCATENATED MODULE: ./src/js/helpers.js
const removeElement = el => {
  if (typeof el.remove !== 'undefined') {
    el.remove();
  } else {
    el.parentNode.removeChild(el);
  }
};


;// CONCATENATED MODULE: ./src/js/timer.js
// https://stackoverflow.com/a/3969760
class Timer {
  constructor(callback, delay) {
    this.startedAt = Date.now();
    this.callback = callback;
    this.delay = delay;
    this.timer = setTimeout(callback, delay);
  }

  pause() {
    this.stop();
    this.delay -= Date.now() - this.startedAt;
  }

  resume() {
    this.stop();
    this.startedAt = Date.now();
    this.timer = setTimeout(this.callback, this.delay);
  }

  stop() {
    clearTimeout(this.timer);
  }

}
;// CONCATENATED MODULE: ./src/js/positions.js
/* harmony default export */ const positions = (Object.freeze({
  TOP_RIGHT: 'top-right',
  TOP: 'top',
  TOP_LEFT: 'top-left',
  BOTTOM_RIGHT: 'bottom-right',
  BOTTOM: 'bottom',
  BOTTOM_LEFT: 'bottom-left'
}));
;// CONCATENATED MODULE: ./node_modules/mitt/dist/mitt.es.js
/* harmony default export */ function mitt_es(n){return{all:n=n||new Map,on:function(t,e){var i=n.get(t);i&&i.push(e)||n.set(t,[e])},off:function(t,e){var i=n.get(t);i&&i.splice(i.indexOf(e)>>>0,1)},emit:function(t,e){(n.get(t)||[]).slice().map(function(n){n(e)}),(n.get("*")||[]).slice().map(function(n){n(t,e)})}}}
//# sourceMappingURL=mitt.es.js.map

;// CONCATENATED MODULE: ./src/js/bus.js

/* harmony default export */ const bus = (mitt_es());
;// CONCATENATED MODULE: ./node_modules/babel-loader/lib/index.js!./node_modules/vue-loader/lib/index.js??vue-loader-options!./src/js/Component.vue?vue&type=script&lang=js&
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//




/* harmony default export */ const Componentvue_type_script_lang_js_ = ({
  name: 'toast',
  props: {
    message: {
      type: String,
      required: true
    },
    type: {
      type: String,
      default: 'success'
    },
    position: {
      type: String,
      default: positions.BOTTOM_RIGHT,

      validator(value) {
        return Object.values(positions).includes(value);
      }

    },
    duration: {
      type: Number,
      default: 3000
    },
    dismissible: {
      type: Boolean,
      default: true
    },
    onDismiss: {
      type: Function,
      default: () => {}
    },
    onClick: {
      type: Function,
      default: () => {}
    },
    queue: Boolean,
    pauseOnHover: {
      type: Boolean,
      default: true
    }
  },

  data() {
    return {
      isActive: false,
      parentTop: null,
      parentBottom: null,
      isHovered: false
    };
  },

  beforeMount() {
    this.setupContainer();
  },

  mounted() {
    this.showNotice();
    bus.on('toast-clear', this.dismiss);
  },

  methods: {
    setupContainer() {
      this.parentTop = document.querySelector('.v-toast.v-toast--top');
      this.parentBottom = document.querySelector('.v-toast.v-toast--bottom'); // No need to create them, they already exists

      if (this.parentTop && this.parentBottom) return;

      if (!this.parentTop) {
        this.parentTop = document.createElement('div');
        this.parentTop.className = 'v-toast v-toast--top';
      }

      if (!this.parentBottom) {
        this.parentBottom = document.createElement('div');
        this.parentBottom.className = 'v-toast v-toast--bottom';
      }

      const container = document.body;
      container.appendChild(this.parentTop);
      container.appendChild(this.parentBottom);
    },

    shouldQueue() {
      if (!this.queue) return false;
      return this.parentTop.childElementCount > 0 || this.parentBottom.childElementCount > 0;
    },

    dismiss() {
      if (this.timer) this.timer.stop();
      clearTimeout(this.queueTimer);
      this.isActive = false; // Timeout for the animation complete before destroying

      setTimeout(() => {
        this.onDismiss.apply(null, arguments);
        this.$destroy();
        removeElement(this.$el);
      }, 150);
    },

    showNotice() {
      if (this.shouldQueue()) {
        // Call recursively if should queue
        this.queueTimer = setTimeout(this.showNotice, 250);
        return;
      }

      this.correctParent.insertAdjacentElement('afterbegin', this.$el);
      this.isActive = true;

      if (this.duration) {
        this.timer = new Timer(this.dismiss, this.duration);
      }
    },

    whenClicked() {
      if (!this.dismissible) return;
      this.onClick.apply(null, arguments);
      this.dismiss();
    },

    toggleTimer(newVal) {
      if (!this.pauseOnHover || !this.timer) return;
      newVal ? this.timer.pause() : this.timer.resume();
    }

  },
  computed: {
    correctParent() {
      switch (this.position) {
        case positions.TOP:
        case positions.TOP_RIGHT:
        case positions.TOP_LEFT:
          return this.parentTop;

        case positions.BOTTOM:
        case positions.BOTTOM_RIGHT:
        case positions.BOTTOM_LEFT:
          return this.parentBottom;
      }
    },

    transition() {
      switch (this.position) {
        case positions.TOP:
        case positions.TOP_RIGHT:
        case positions.TOP_LEFT:
          return {
            enter: 'v-toast--fade-in-down',
            leave: 'v-toast--fade-out'
          };

        case positions.BOTTOM:
        case positions.BOTTOM_RIGHT:
        case positions.BOTTOM_LEFT:
          return {
            enter: 'v-toast--fade-in-up',
            leave: 'v-toast--fade-out'
          };
      }
    }

  },

  beforeDestroy() {
    bus.off('toast-clear', this.dismiss);
  }

});
;// CONCATENATED MODULE: ./src/js/Component.vue?vue&type=script&lang=js&
 /* harmony default export */ const js_Componentvue_type_script_lang_js_ = (Componentvue_type_script_lang_js_); 
;// CONCATENATED MODULE: ./node_modules/vue-loader/lib/runtime/componentNormalizer.js
/* globals __VUE_SSR_CONTEXT__ */

// IMPORTANT: Do NOT use ES2015 features in this file (except for modules).
// This module is a runtime utility for cleaner component module output and will
// be included in the final webpack user bundle.

function normalizeComponent (
  scriptExports,
  render,
  staticRenderFns,
  functionalTemplate,
  injectStyles,
  scopeId,
  moduleIdentifier, /* server only */
  shadowMode /* vue-cli only */
) {
  // Vue.extend constructor export interop
  var options = typeof scriptExports === 'function'
    ? scriptExports.options
    : scriptExports

  // render functions
  if (render) {
    options.render = render
    options.staticRenderFns = staticRenderFns
    options._compiled = true
  }

  // functional template
  if (functionalTemplate) {
    options.functional = true
  }

  // scopedId
  if (scopeId) {
    options._scopeId = 'data-v-' + scopeId
  }

  var hook
  if (moduleIdentifier) { // server build
    hook = function (context) {
      // 2.3 injection
      context =
        context || // cached call
        (this.$vnode && this.$vnode.ssrContext) || // stateful
        (this.parent && this.parent.$vnode && this.parent.$vnode.ssrContext) // functional
      // 2.2 with runInNewContext: true
      if (!context && typeof __VUE_SSR_CONTEXT__ !== 'undefined') {
        context = __VUE_SSR_CONTEXT__
      }
      // inject component styles
      if (injectStyles) {
        injectStyles.call(this, context)
      }
      // register component module identifier for async chunk inferrence
      if (context && context._registeredComponents) {
        context._registeredComponents.add(moduleIdentifier)
      }
    }
    // used by ssr in case component is cached and beforeCreate
    // never gets called
    options._ssrRegister = hook
  } else if (injectStyles) {
    hook = shadowMode
      ? function () {
        injectStyles.call(
          this,
          (options.functional ? this.parent : this).$root.$options.shadowRoot
        )
      }
      : injectStyles
  }

  if (hook) {
    if (options.functional) {
      // for template-only hot-reload because in that case the render fn doesn't
      // go through the normalizer
      options._injectStyles = hook
      // register for functional component in vue file
      var originalRender = options.render
      options.render = function renderWithStyleInjection (h, context) {
        hook.call(context)
        return originalRender(h, context)
      }
    } else {
      // inject component registration as beforeCreate hook
      var existing = options.beforeCreate
      options.beforeCreate = existing
        ? [].concat(existing, hook)
        : [hook]
    }
  }

  return {
    exports: scriptExports,
    options: options
  }
}

;// CONCATENATED MODULE: ./src/js/Component.vue





/* normalize component */
;
var component = normalizeComponent(
  js_Componentvue_type_script_lang_js_,
  render,
  staticRenderFns,
  false,
  null,
  null,
  null
  
)

/* harmony default export */ const Component = (component.exports);
;// CONCATENATED MODULE: ./src/js/api.js



const Api = (Vue, globalOptions = {}) => {
  return {
    open(options) {
      let message;
      if (typeof options === 'string') message = options;
      const defaultOptions = {
        message
      };
      const propsData = Object.assign({}, defaultOptions, globalOptions, options);
      return new (Vue.extend(Component))({
        el: document.createElement('div'),
        propsData
      });
    },

    clear() {
      bus.emit('toast-clear');
    },

    success(message, options = {}) {
      return this.open(Object.assign({}, {
        message,
        type: 'success'
      }, options));
    },

    error(message, options = {}) {
      return this.open(Object.assign({}, {
        message,
        type: 'error'
      }, options));
    },

    info(message, options = {}) {
      return this.open(Object.assign({}, {
        message,
        type: 'info'
      }, options));
    },

    warning(message, options = {}) {
      return this.open(Object.assign({}, {
        message,
        type: 'warning'
      }, options));
    },

    default(message, options = {}) {
      return this.open(Object.assign({}, {
        message,
        type: 'default'
      }, options));
    }

  };
};

/* harmony default export */ const api = (Api);
;// CONCATENATED MODULE: ./src/index.js




const Plugin = (Vue, options = {}) => {
  let methods = api(Vue, options);
  Vue.$toast = methods;
  Vue.prototype.$toast = methods;
};

Component.install = Plugin;
/* harmony default export */ const src = (Component);


/***/ })

/******/ 	});
/************************************************************************/
/******/ 	// The module cache
/******/ 	var __webpack_module_cache__ = {};
/******/ 	
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/ 		// Check if module is in cache
/******/ 		if(__webpack_module_cache__[moduleId]) {
/******/ 			return __webpack_module_cache__[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = __webpack_module_cache__[moduleId] = {
/******/ 			// no module.id needed
/******/ 			// no module.loaded needed
/******/ 			exports: {}
/******/ 		};
/******/ 	
/******/ 		// Execute the module function
/******/ 		__webpack_modules__[moduleId](module, module.exports, __webpack_require__);
/******/ 	
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/ 	
/************************************************************************/
/******/ 	/* webpack/runtime/define property getters */
/******/ 	(() => {
/******/ 		// define getter functions for harmony exports
/******/ 		__webpack_require__.d = (exports, definition) => {
/******/ 			for(var key in definition) {
/******/ 				if(__webpack_require__.o(definition, key) && !__webpack_require__.o(exports, key)) {
/******/ 					Object.defineProperty(exports, key, { enumerable: true, get: definition[key] });
/******/ 				}
/******/ 			}
/******/ 		};
/******/ 	})();
/******/ 	
/******/ 	/* webpack/runtime/hasOwnProperty shorthand */
/******/ 	(() => {
/******/ 		__webpack_require__.o = (obj, prop) => Object.prototype.hasOwnProperty.call(obj, prop)
/******/ 	})();
/******/ 	
/************************************************************************/
/******/ 	// module exports must be returned from runtime so entry inlining is disabled
/******/ 	// startup
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(6);
/******/ })()
.default;
});