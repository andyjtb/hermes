function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function"); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, writable: true, configurable: true } }); Object.defineProperty(subClass, "prototype", { writable: false }); if (superClass) _setPrototypeOf(subClass, superClass); }
function _setPrototypeOf(o, p) { _setPrototypeOf = Object.setPrototypeOf ? Object.setPrototypeOf.bind() : function _setPrototypeOf(o, p) { o.__proto__ = p; return o; }; return _setPrototypeOf(o, p); }
function _createSuper(Derived) { var hasNativeReflectConstruct = _isNativeReflectConstruct(); return function _createSuperInternal() { var Super = _getPrototypeOf(Derived), result; if (hasNativeReflectConstruct) { var NewTarget = _getPrototypeOf(this).constructor; result = Reflect.construct(Super, arguments, NewTarget); } else { result = Super.apply(this, arguments); } return _possibleConstructorReturn(this, result); }; }
function _possibleConstructorReturn(self, call) { if (call && (typeof call === "object" || typeof call === "function")) { return call; } else if (call !== void 0) { throw new TypeError("Derived constructors may only return object or undefined"); } return _assertThisInitialized(self); }
function _assertThisInitialized(self) { if (self === void 0) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return self; }
function _isNativeReflectConstruct() { try { var t = !Boolean.prototype.valueOf.call(Reflect.construct(Boolean, [], function () {})); } catch (t) {} return (_isNativeReflectConstruct = function _isNativeReflectConstruct() { return !!t; })(); }
function _getPrototypeOf(o) { _getPrototypeOf = Object.setPrototypeOf ? Object.getPrototypeOf.bind() : function _getPrototypeOf(o) { return o.__proto__ || Object.getPrototypeOf(o); }; return _getPrototypeOf(o); }
function _slicedToArray(arr, i) { return _arrayWithHoles(arr) || _iterableToArrayLimit(arr, i) || _unsupportedIterableToArray(arr, i) || _nonIterableRest(); }
function _nonIterableRest() { throw new TypeError("Invalid attempt to destructure non-iterable instance.\nIn order to be iterable, non-array objects must have a [Symbol.iterator]() method."); }
function _unsupportedIterableToArray(o, minLen) { if (!o) return; if (typeof o === "string") return _arrayLikeToArray(o, minLen); var n = Object.prototype.toString.call(o).slice(8, -1); if (n === "Object" && o.constructor) n = o.constructor.name; if (n === "Map" || n === "Set") return Array.from(o); if (n === "Arguments" || /^(?:Ui|I)nt(?:8|16|32)(?:Clamped)?Array$/.test(n)) return _arrayLikeToArray(o, minLen); }
function _arrayLikeToArray(arr, len) { if (len == null || len > arr.length) len = arr.length; for (var i = 0, arr2 = new Array(len); i < len; i++) arr2[i] = arr[i]; return arr2; }
function _iterableToArrayLimit(r, l) { var t = null == r ? null : "undefined" != typeof Symbol && r[Symbol.iterator] || r["@@iterator"]; if (null != t) { var e, n, i, u, a = [], f = !0, o = !1; try { if (i = (t = t.call(r)).next, 0 === l) { if (Object(t) !== t) return; f = !1; } else for (; !(f = (e = i.call(t)).done) && (a.push(e.value), a.length !== l); f = !0); } catch (r) { o = !0, n = r; } finally { try { if (!f && null != t.return && (u = t.return(), Object(u) !== u)) return; } finally { if (o) throw n; } } return a; } }
function _arrayWithHoles(arr) { if (Array.isArray(arr)) return arr; }
function _defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, _toPropertyKey(descriptor.key), descriptor); } }
function _createClass(Constructor, protoProps, staticProps) { if (protoProps) _defineProperties(Constructor.prototype, protoProps); if (staticProps) _defineProperties(Constructor, staticProps); Object.defineProperty(Constructor, "prototype", { writable: false }); return Constructor; }
function _toPropertyKey(t) { var i = _toPrimitive(t, "string"); return "symbol" == typeof i ? i : String(i); }
function _toPrimitive(t, r) { if ("object" != typeof t || !t) return t; var e = t[Symbol.toPrimitive]; if (void 0 !== e) { var i = e.call(t, r || "default"); if ("object" != typeof i) return i; throw new TypeError("@@toPrimitive must return a primitive value."); } return ("string" === r ? String : Number)(t); }
function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }
/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow
 * @generated
 *
 * Entrypoints:
 *   app/simple/index.js
 */
/* file: packages/react/invariant.js */
function M$react_invariant$default(condition, format) {
  'inline';

  if (!condition) {
    throw new Error(format);
  }
}
/* file: packages/sh/CHECKED_CAST.js */
function M$sh_CHECKED_CAST$default(value) {
  'inline';

  return value;
}
/* file: packages/sh/microtask.js */
var M$sh_microtask$INTERNAL$microtaskQueue = [];
function M$sh_microtask$drainMicrotaskQueue() {
  for (var i = 0; i < M$sh_microtask$INTERNAL$microtaskQueue.length; i++) {
    M$sh_microtask$INTERNAL$microtaskQueue[i]();
    M$sh_microtask$INTERNAL$microtaskQueue[i] = undefined;
  }
  M$sh_microtask$INTERNAL$microtaskQueue = [];
}
function M$sh_microtask$queueMicrotask(callback) {
  M$sh_microtask$INTERNAL$microtaskQueue.push(callback);
}
/* file: packages/sh/fastarray.js */
function M$sh_fastarray$join(arr, sep) {
  var result = '';
  for (var i = 0, e = arr.length; i < e; ++i) {
    if (i !== 0) result += sep;
    result += arr[i];
  }
  return result;
}
function M$sh_fastarray$reduce(arr, fn, initialValue) {
  var acc = initialValue;
  for (var i = 0, e = arr.length; i < e; ++i) {
    acc = fn(acc, arr[i], i);
  }
  return acc;
}
function M$sh_fastarray$map(arr, fn) {
  var output = [];
  for (var i = 0, e = arr.length; i < e; ++i) {
    output.push(fn(arr[i], i));
  }
  return output;
}
function M$sh_fastarray$includes(arr, searchElement) {
  for (var i = 0, e = arr.length; i < e; ++i) {
    if (arr[i] === searchElement) {
      return true;
    }
  }
  return false;
}
/* file: packages/react/index.js */
function M$react_index$INTERNAL$padString(str, len) {
  var result = '';
  for (var i = 0; i < len; i++) {
    result += str;
  }
  return result;
}

/**
 * The type of an element in React. A React element may be a:
 *
 * - String. These elements are intrinsics that depend on the React renderer
 *   implementation.
 * - React component. See `ComponentType` for more information about its
 *   different variants.
 */
/**
 * Type of a React element. React elements are commonly created using JSX
 * literals, which desugar to React.createElement calls (see below).
 */
// type React$Element<ElementType: React$ElementType> = {|
//   type: ElementType,
//   props: Props,
//   key: React$Key | null,
//   ref: any,
// |};
var M$react_index$INTERNAL$React$Element = /*#__PURE__*/_createClass(function M$react_index$INTERNAL$React$Element(type, props, key, ref) {
  "use strict";

  _classCallCheck(this, M$react_index$INTERNAL$React$Element);
  this.type = type;
  this.props = props;
  this.key = key;
  this.ref = ref;
});
/**
 * The type of the key that React uses to determine where items in a new list
 * have moved.
 */
var M$react_index$INTERNAL$REACT_FRAGMENT_TYPE = 1 /* Symbol.for('react.fragment') */;
/* eslint-disable lint/strictly-null, lint/react-state-props-mutation, lint/flow-react-element */

/**
 * The current root
 */
var M$react_index$INTERNAL$workInProgressRoot = null;
/**
 * The currently rendering fiber. Only set when a component is being rendered.
 */
var M$react_index$INTERNAL$workInProgressFiber = null;
/**
 * The previous state hook, or null if no state hook has been evaluated yet.
 */
var M$react_index$INTERNAL$workInProgressState = null;
/**
 * Queue of updates triggered *during* render.
 */
var M$react_index$INTERNAL$renderPhaseUpdateQueue = [];
/**
 * Public API to create a new "root", this is where React attaches rendering to a host element.
 * In our case we don't actually have a real host, and currently only "render" to strings.
 */
function M$react_index$createRoot() {
  return new M$react_index$INTERNAL$Root();
}
/**
 * Hook to create (on initial render) or access (on update) a state, using the index of the useState
 * call within the component as the identity. Thus conditionally calling this API can cause state to
 * be lost.
 */
function M$react_index$useState(
/**
 * Initial value of the state
 */
initial) {
  var root = M$sh_CHECKED_CAST$default(M$react_index$INTERNAL$workInProgressRoot);
  var fiber = M$sh_CHECKED_CAST$default(M$react_index$INTERNAL$workInProgressFiber);
  M$react_invariant$default(fiber !== null && root !== null, 'useState() called outside of render');
  var state;
  var _workInProgressState = M$react_index$INTERNAL$workInProgressState;
  if (_workInProgressState === null) {
    // Get or initialize the first state on the fiber
    var nextState = fiber.state;
    if (nextState === null) {
      nextState = new M$react_index$INTERNAL$State(initial);
      fiber.state = nextState;
    }
    // NOTE: in case of a re-render we assume that the hook types match but
    // can't statically prove this
    state = M$sh_CHECKED_CAST$default(nextState);
  } else {
    var _nextState = M$sh_CHECKED_CAST$default(_workInProgressState).next;
    if (_nextState === null) {
      _nextState = new M$react_index$INTERNAL$State(initial);
      M$sh_CHECKED_CAST$default(_workInProgressState).next = _nextState;
    }
    // NOTE: in case of a re-render we assume that the hook types match but
    // can't statically prove this
    state = M$sh_CHECKED_CAST$default(_nextState);
  }
  // NOTE: this should just work because of subtying, State<T> should be subtype of State<mixed>
  M$react_index$INTERNAL$workInProgressState = M$sh_CHECKED_CAST$default(state);
  return [
  // Untyped check that the existing state value has the correct type,
  // This is safe if components follow the rules of hooks
  M$sh_CHECKED_CAST$default(state.value), function (updater) {
    var update = new M$react_index$INTERNAL$Update(fiber, M$sh_CHECKED_CAST$default(state), M$sh_CHECKED_CAST$default(updater));
    if (M$react_index$INTERNAL$workInProgressFiber !== null) {
      // called during render
      M$react_index$INTERNAL$renderPhaseUpdateQueue.push(update);
    } else {
      root.notify(update);
    }
  }];
}
var M$react_index$INTERNAL$callbacks = new Map();
function M$react_index$callOnClickOrChange(id, event) {
  var callback = M$react_index$INTERNAL$callbacks.get(id);
  if (callback == null) {
    throw new Error('No callback registered with id: ' + id);
  }
  callback(event);
}

/**
 * The type of value that may be passed to the setState function (second part of useState return value).
 * - T: the new value
 * - (prev: T) => T: a function to compute the new value from the old value
 */

/**
 * The type of the setState function (second element of the array returned by useState).
 */
/**
 * A queued state update.
 */
var M$react_index$INTERNAL$Update = /*#__PURE__*/function () {
  "use strict";

  function M$react_index$INTERNAL$Update(fiber, state, updater) {
    _classCallCheck(this, M$react_index$INTERNAL$Update);
    this.fiber = fiber;
    this.state = state;
    this.updater = updater;
  }
  _createClass(M$react_index$INTERNAL$Update, [{
    key: "run",
    value: function run() {
      var state = this.state;
      var value = state.value;
      var updater = this.updater;
      if (typeof updater === 'function') {
        // NOTE: The type of Updater<T> is meant to expresss `T (not function) | T (function of T => T)`
        // thus the fact that updater is a function here menas its a function of T => T.
        var fn = M$sh_CHECKED_CAST$default(updater);
        value = fn(state.value);
      } else {
        // NOTE: The type of Updater<T> is meant to expresss `T (not function) | T (function of T => T)`
        // thus the fact that updater is *not* a function here means it is a T
        value = M$sh_CHECKED_CAST$default(updater);
      }
      var changed = !Object.is(state.value, value);
      state.value = value;
      return changed;
    }
  }]);
  return M$react_index$INTERNAL$Update;
}();
var M$react_index$INTERNAL$Root = /*#__PURE__*/function () {
  "use strict";

  function M$react_index$INTERNAL$Root() {
    _classCallCheck(this, M$react_index$INTERNAL$Root);
    this.root = null;
    this.element = null;
    this.updateQueue = [];
  }
  _createClass(M$react_index$INTERNAL$Root, [{
    key: "notify",
    value: function notify(update) {
      var _this = this;
      this.updateQueue.push(update);
      if (this.updateQueue.length === 1) {
        M$sh_microtask$queueMicrotask(function () {
          var element = _this.element;
          M$react_invariant$default(element !== null, 'Expected an element to be set after rendering');
          _this.doWork(M$sh_CHECKED_CAST$default(element));
        });
      }
    }
  }, {
    key: "render",
    value: function render(element) {
      M$react_invariant$default(M$react_index$INTERNAL$workInProgressFiber === null && M$react_index$INTERNAL$workInProgressState === null, 'Cannot render, an existing render is in progress');
      var hasChanges = element !== this.element;
      this.element = element;
      if (hasChanges) {
        this.doWork(element);
      }
      M$react_invariant$default(this.root !== null, 'Expected root to be rendered');
      var root = M$sh_CHECKED_CAST$default(this.root);
      var output = [];
      this.printFiber(root, output, 0);
      return M$sh_fastarray$join(output, '\n');
    }
  }, {
    key: "doWork",
    value: function doWork(element) {
      var mustRender = this.root === null;
      for (var update of this.updateQueue) {
        mustRender = update.run() || mustRender;
      }
      this.updateQueue = [];
      if (!mustRender) {
        return;
      }
      // Visit the tree in pre-order, rendering each node
      // and then processing its children
      // eslint-disable-next-line consistent-this
      M$react_index$INTERNAL$workInProgressRoot = this;
      var fiber = this.root;
      if (fiber === null) {
        fiber = this.mountFiber(element, null);
        this.root = fiber;
      }
      while (fiber !== null) {
        // Render the fiber, which creates child/sibling nodes
        var fiber2 = M$sh_CHECKED_CAST$default(fiber);
        this.renderFiber(fiber2);
        // advance to the next fiber
        if (fiber2.child !== null) {
          fiber = fiber2.child;
        } else if (fiber2.sibling !== null) {
          fiber = fiber2.sibling;
        } else {
          fiber = fiber2.parent;
          while (fiber !== null && M$sh_CHECKED_CAST$default(fiber).sibling === null) {
            fiber = M$sh_CHECKED_CAST$default(fiber).parent;
          }
          if (fiber !== null) {
            fiber = M$sh_CHECKED_CAST$default(fiber).sibling;
          }
        }
      }
      M$react_index$INTERNAL$workInProgressRoot = null;
    }
  }, {
    key: "printFiber",
    value: function printFiber(fiber, out, level) {
      switch (fiber.type.kind) {
        case 'host':
          {
            var tag = M$sh_CHECKED_CAST$default(fiber.type).tag;
            var padStr = M$react_index$INTERNAL$padString(' ', level);
            var str = padStr + '<' + tag;
            for (var _ref of Object.entries(fiber.props)) {
              var _JSON$stringify;
              var _ref2 = _slicedToArray(_ref, 2);
              var propName = _ref2[0];
              var propValue = _ref2[1];
              if (propValue == null || typeof propValue === 'function') {
                continue;
              }
              str += ` ${propName}=${(_JSON$stringify = JSON.stringify(propValue)) != null ? _JSON$stringify : 'undefined'}`;
            }
            if (fiber.child == null) {
              str += ' />';
              out.push(str);
            } else {
              str += '>';
              out.push(str);
              this.printChildren(fiber, out, level + 1);
              out.push(padStr + '</' + tag + '>');
            }
            break;
          }
        case 'text':
          {
            var text = M$sh_CHECKED_CAST$default(fiber.type).text;
            if (text !== '') {
              out.push(M$react_index$INTERNAL$padString(' ', level) + text);
            }
            break;
          }
        case 'fragment':
        case 'component':
          {
            this.printChildren(fiber, out, level);
            break;
          }
      }
    }
  }, {
    key: "printChildren",
    value: function printChildren(fiber, out, level) {
      var current = fiber.child;
      while (current !== null) {
        this.printFiber(M$sh_CHECKED_CAST$default(current), out, level);
        current = M$sh_CHECKED_CAST$default(current).sibling;
      }
    }
  }, {
    key: "renderFiber",
    value: function renderFiber(fiber) {
      try {
        M$react_index$INTERNAL$workInProgressFiber = fiber;
        M$react_index$INTERNAL$workInProgressState = null;
        switch (fiber.type.kind) {
          case 'component':
            {
              M$react_invariant$default(M$react_index$INTERNAL$renderPhaseUpdateQueue.length === 0, 'Expected no queued render updates');
              var render = M$sh_CHECKED_CAST$default(fiber.type).component;
              var element = render(fiber.props);
              var iterationCount = 0;
              while (M$react_index$INTERNAL$renderPhaseUpdateQueue.length !== 0) {
                iterationCount++;
                M$react_invariant$default(iterationCount < 1000, 'Possible infinite loop with setState during render');
                var hasChanges = false;
                for (var update of M$react_index$INTERNAL$renderPhaseUpdateQueue) {
                  M$react_invariant$default(update.fiber === fiber, 'setState() during render is currently only supported when updating the component ' + 'being rendered. Setting state from another component is not supported.');
                  hasChanges = update.run() || hasChanges;
                }
                M$react_index$INTERNAL$renderPhaseUpdateQueue.length = 0;
                if (!hasChanges) {
                  break;
                }
                element = render(fiber.props);
              }
              fiber.child = this.reconcileFiber(fiber, fiber.child, element);
              break;
            }
          case 'host':
            {
              var id = fiber.props.id;
              if (id != null) {
                var onClick = fiber.props.onClick;
                if (onClick != null) {
                  M$react_index$INTERNAL$callbacks.set(id, onClick);
                }
                var onChange = fiber.props.onChange;
                if (onChange != null) {
                  M$react_index$INTERNAL$callbacks.set(id, onChange);
                }
              }
              break;
            }
          case 'fragment':
          case 'text':
            {
              // Nothing to reconcile, these nodes are visited by the main doWork() loop
              break;
            }
          default:
            {
              throw new Error('Unexpected fiber kind: ' + fiber.type.kind);
            }
        }
      } finally {
        M$react_index$INTERNAL$workInProgressFiber = null;
        M$react_index$INTERNAL$workInProgressState = null;
      }
    }
  }, {
    key: "mountFiber",
    value: function mountFiber(elementOrString, parent) {
      var fiber;
      // TODO: Support Array of Node's being returned from a component.
      if (typeof elementOrString === 'object') {
        var element = M$sh_CHECKED_CAST$default(elementOrString);
        if (typeof element.type === 'function') {
          var component = M$sh_CHECKED_CAST$default(element.type);
          var type = new M$react_index$INTERNAL$FiberTypeComponent(component);
          fiber = new M$react_index$INTERNAL$Fiber(type, element.props, element.key);
        } else if (typeof element.type === 'string') {
          M$react_invariant$default(typeof element.type === 'string', 'Expected a host component name such as "div" or "span", got ' + typeof element.type);
          var _type = new M$react_index$INTERNAL$FiberTypeHost(M$sh_CHECKED_CAST$default(element.type));
          M$react_invariant$default(element.props !== null && typeof element.props === 'object', 'Expected component props');
          // const {children, ...props} = element.props;
          var children = element.props.children;
          var _props = Object.assign({}, element.props);
          delete _props.children;
          fiber = new M$react_index$INTERNAL$Fiber(_type, _props, element.key);
          this.mountChildren(children, fiber);
        } else {
          switch (element.type) {
            case M$react_index$INTERNAL$REACT_FRAGMENT_TYPE:
              {
                var _type2 = new M$react_index$INTERNAL$FiberTypeFragment();
                fiber = new M$react_index$INTERNAL$Fiber(_type2, element.props, element.key);
                this.mountChildren(element.props.children, fiber);
                break;
              }
            default:
              {
                throw new Error(`Unknown element type ${element.type}`);
              }
          }
        }
      } else if (typeof elementOrString === 'string') {
        var _type3 = new M$react_index$INTERNAL$FiberTypeText(M$sh_CHECKED_CAST$default(elementOrString));
        fiber = new M$react_index$INTERNAL$Fiber(_type3, {}, null);
      } else {
        throw new Error(`Unexpected element type of ${typeof elementOrString}`);
      }
      fiber.parent = parent;
      return fiber;
    }
  }, {
    key: "mountChildren",
    value: function mountChildren(children, parentFiber) {
      if (Array.isArray(children)) {
        var _prev = null;
        for (var childElement of M$sh_CHECKED_CAST$default(children)) {
          if (childElement == null) {
            continue;
          }
          var child = this.mountFiber(M$sh_CHECKED_CAST$default(childElement), parentFiber);
          if (_prev !== null) {
            M$sh_CHECKED_CAST$default(_prev).sibling = child;
          } else {
            // set parent to point to first child
            parentFiber.child = child;
          }
          _prev = child;
        }
      } else if (children != null) {
        var _child = this.mountFiber(children, parentFiber);
        parentFiber.child = _child;
      }
    }
  }, {
    key: "reconcileFiber",
    value: function reconcileFiber(parent, prevChild, element) {
      if (prevChild !== null && M$sh_CHECKED_CAST$default(prevChild).type === element.type) {
        var _prevChild = M$sh_CHECKED_CAST$default(_prevChild);
        // Only host and fragment nodes have to be reconciled: otherwise this is a
        // function component and its children will be reconciled when they are later
        // emitted in a host position (ie as a direct result of render)
        switch (_prevChild.type.kind) {
          case 'host':
            {
              M$react_invariant$default(element.props !== null && typeof element.props === 'object', 'Expected component props');
              // const {children, ...props} = element.props;
              var children = element.props.children;
              var _props2 = Object.assign({}, element.props);
              delete _props2.children;
              _prevChild.props = _props2;
              this.reconcileChildren(_prevChild, children);
              break;
            }
          case 'fragment':
            {
              M$react_invariant$default(element.props !== null && typeof element.props === 'object', 'Expected component props');
              var _children = element.props.children;
              this.reconcileChildren(_prevChild, _children);
              break;
            }
          case 'component':
            {
              M$react_invariant$default(element.props !== null && typeof element.props === 'object', 'Expected component props');
              _prevChild.props = element.props;
              break;
            }
          default:
            {
              throw new Error(`Unknown node kind ${_prevChild.type.kind}`);
            }
        }
        return _prevChild;
      } else {
        var child = this.mountFiber(element, parent);
        return child;
      }
    }
  }, {
    key: "reconcileChildren",
    value: function reconcileChildren(parent, children) {
      var prevChild = parent.child;
      if (Array.isArray(children)) {
        var childrenArray = M$sh_CHECKED_CAST$default(children);
        // Fast-path for empty and single-element arrays
        if (childrenArray.length === 0) {
          parent.child = null;
        } else if (childrenArray.length === 1) {
          parent.child = this.reconcileFiber(parent, prevChild, childrenArray[0]);
          M$sh_CHECKED_CAST$default(parent.child).sibling = null;
        } else {
          this.reconcileMultipleChildren(parent, childrenArray);
        }
      } else if (typeof children === 'string') {
        if (prevChild === null || M$sh_CHECKED_CAST$default(prevChild).type.kind !== 'text') {
          var type = new M$react_index$INTERNAL$FiberTypeText(M$sh_CHECKED_CAST$default(children));
          var child = new M$react_index$INTERNAL$Fiber(type, {}, null);
          parent.child = child;
        } else {
          M$sh_CHECKED_CAST$default(M$sh_CHECKED_CAST$default(prevChild).type).text = M$sh_CHECKED_CAST$default(children);
        }
      } else if (children != null) {
        parent.child = this.reconcileFiber(parent, prevChild, M$sh_CHECKED_CAST$default(children));
        M$sh_CHECKED_CAST$default(parent.child).sibling = null;
      } else {
        parent.child = null;
        if (prevChild !== null) {
          M$sh_CHECKED_CAST$default(prevChild).parent = null;
        }
      }
    }
  }, {
    key: "reconcileMultipleChildren",
    value: function reconcileMultipleChildren(parent, children) {
      M$react_invariant$default(children.length > 1, 'Expected children to have multiple elements');
      // map existing children by key to make subsequent lookup O(log n)
      var keyedChildren = new Map();
      var current = parent.child;
      while (current !== null) {
        if (M$sh_CHECKED_CAST$default(current).key !== null) {
          keyedChildren.set(M$sh_CHECKED_CAST$default(current).key, current);
        }
        current = M$sh_CHECKED_CAST$default(current).sibling;
      }
      var prev = null; // previous fiber at this key/index
      var prevByIndex = parent.child; // keep track of prev fiber at this index
      for (var childElement of children) {
        var _ref3;
        var prevFiber = (_ref3 = childElement.key != null ? keyedChildren.get(childElement.key) : null) != null ? _ref3 : prevByIndex;
        var child = void 0;
        if (prevFiber != null) {
          child = this.reconcileFiber(parent, prevFiber, childElement);
        } else {
          child = this.mountFiber(childElement, parent);
        }
        if (prev !== null) {
          M$sh_CHECKED_CAST$default(prev).sibling = child;
        } else {
          // set parent to point to first child
          parent.child = child;
        }
        prev = child;
        prevByIndex = prevByIndex !== null ? M$sh_CHECKED_CAST$default(prevByIndex).sibling : null;
      }
    }
  }]);
  return M$react_index$INTERNAL$Root;
}();
/**
 * Describes the `type` field of Fiber, which can hold different data depending on the fiber's kind:
 * - Component stores a function of props => element.
 * - Host stores the name of the host component, ie "div"
 * - Text stores the text itself.
 */
// type FiberType =
//   | {
//       kind: 'component',
//       component: Component,
//     }
//   | {
//       kind: 'host',
//       tag: string,
//     }
//   | {
//       kind: 'text',
//       text: string,
//     };
var M$react_index$INTERNAL$FiberType = /*#__PURE__*/_createClass(function M$react_index$INTERNAL$FiberType(kind) {
  "use strict";

  _classCallCheck(this, M$react_index$INTERNAL$FiberType);
  this.kind = kind;
});
var M$react_index$INTERNAL$FiberTypeComponent = /*#__PURE__*/function (_M$react_index$INTERN) {
  "use strict";

  _inherits(M$react_index$INTERNAL$FiberTypeComponent, _M$react_index$INTERN);
  var _super = _createSuper(M$react_index$INTERNAL$FiberTypeComponent);
  function M$react_index$INTERNAL$FiberTypeComponent(component) {
    var _this2;
    _classCallCheck(this, M$react_index$INTERNAL$FiberTypeComponent);
    _this2 = _super.call(this, 'component');
    _this2.component = component;
    return _this2;
  }
  return _createClass(M$react_index$INTERNAL$FiberTypeComponent);
}(M$react_index$INTERNAL$FiberType);
var M$react_index$INTERNAL$FiberTypeHost = /*#__PURE__*/function (_M$react_index$INTERN2) {
  "use strict";

  _inherits(M$react_index$INTERNAL$FiberTypeHost, _M$react_index$INTERN2);
  var _super2 = _createSuper(M$react_index$INTERNAL$FiberTypeHost);
  function M$react_index$INTERNAL$FiberTypeHost(tag) {
    var _this3;
    _classCallCheck(this, M$react_index$INTERNAL$FiberTypeHost);
    _this3 = _super2.call(this, 'host');
    _this3.tag = tag;
    return _this3;
  }
  return _createClass(M$react_index$INTERNAL$FiberTypeHost);
}(M$react_index$INTERNAL$FiberType);
var M$react_index$INTERNAL$FiberTypeFragment = /*#__PURE__*/function (_M$react_index$INTERN3) {
  "use strict";

  _inherits(M$react_index$INTERNAL$FiberTypeFragment, _M$react_index$INTERN3);
  var _super3 = _createSuper(M$react_index$INTERNAL$FiberTypeFragment);
  function M$react_index$INTERNAL$FiberTypeFragment() {
    _classCallCheck(this, M$react_index$INTERNAL$FiberTypeFragment);
    return _super3.call(this, 'fragment');
  }
  return _createClass(M$react_index$INTERNAL$FiberTypeFragment);
}(M$react_index$INTERNAL$FiberType);
var M$react_index$INTERNAL$FiberTypeText = /*#__PURE__*/function (_M$react_index$INTERN4) {
  "use strict";

  _inherits(M$react_index$INTERNAL$FiberTypeText, _M$react_index$INTERN4);
  var _super4 = _createSuper(M$react_index$INTERNAL$FiberTypeText);
  function M$react_index$INTERNAL$FiberTypeText(text) {
    var _this4;
    _classCallCheck(this, M$react_index$INTERNAL$FiberTypeText);
    _this4 = _super4.call(this, 'text');
    _this4.text = text;
    return _this4;
  }
  return _createClass(M$react_index$INTERNAL$FiberTypeText);
}(M$react_index$INTERNAL$FiberType);
/**
 * The type of component props as seen by the framework, because processing is heterogenous
 * the framework only looks at the identity of prop values and does not otherwise make any
 * assumptions about which props may exist and what their types are.
 */
/**
 * Data storage for the useState() hook
 */
var M$react_index$INTERNAL$State = /*#__PURE__*/_createClass(function M$react_index$INTERNAL$State(value) {
  "use strict";

  _classCallCheck(this, M$react_index$INTERNAL$State);
  this.value = value;
  this.next = null;
  this.prev = null;
});
/**
 * Represents a node in the UI tree, and may correspond to a user-defined function component,
 * a host node, or a text node.
 */
var M$react_index$INTERNAL$Fiber = /*#__PURE__*/_createClass(function M$react_index$INTERNAL$Fiber(type, props, key) {
  "use strict";

  _classCallCheck(this, M$react_index$INTERNAL$Fiber);
  this.type = type;
  this.props = props;
  this.key = key;
  this.parent = null;
  this.child = null;
  this.sibling = null;
  this.state = null;
});
function M$react_index$jsx(type, props, key) {
  'inline';

  return new M$react_index$INTERNAL$React$Element(type, props, key, null);
}
function M$react_index$Fragment(props) {
  'inline';

  return new M$react_index$INTERNAL$React$Element(M$react_index$INTERNAL$REACT_FRAGMENT_TYPE, props, null, null);
}
function M$react_index$forwardRef(comp) {
  return function (props) {
    return comp(props, null);
  };
}
/* file: app/simple/App.js */
function M$App$INTERNAL$Button(props) {
  return M$react_index$jsx('button', {
    id: props.id,
    onClick: props.onClick,
    children: 'Click me'
  }, null);
}
function M$App$INTERNAL$Input(props) {
  return M$react_index$jsx('input', {
    id: props.id,
    type: "text",
    onChange: props.onChange,
    value: props.value
  }, null);
}
function M$App$INTERNAL$TextArea(props) {
  return M$react_index$jsx('textarea', {
    onChange: props.onChange,
    children: props.value
  }, null);
}
function M$App$INTERNAL$Select(props) {
  var children = [];
  for (var i = 0; i < props.options.length; i++) {
    var option = props.options[i];
    children.push(M$react_index$jsx('option', {
      value: option.value,
      children: option.label
    }, option.value));
  }
  return M$react_index$jsx('select', {
    onChange: props.onChange,
    children: children
  }, null);
}
function M$App$INTERNAL$Checkbox(props) {
  return M$react_index$jsx('input', {
    type: "checkbox",
    checked: props.checked,
    onChange: props.onChange
  }, null);
}
function M$App$INTERNAL$Radio(props) {
  return M$react_index$jsx('input', {
    type: "radio",
    checked: props.checked,
    onChange: props.onChange
  }, null);
}
function M$App$INTERNAL$Slider(props) {
  return M$react_index$jsx('input', {
    type: "range",
    min: props.min,
    max: props.max,
    step: props.step,
    value: props.value,
    onChange: props.onChange
  }, null);
}
function M$App$INTERNAL$ProgressBar(props) {
  return M$react_index$jsx('div', {
    style: {
      width: `${props.progress}%`
    }
  }, null);
}
function M$App$INTERNAL$Spinner(props) {
  return M$react_index$jsx('div', {
    className: "spinner",
    children: 'Loading...'
  }, null);
}
function M$App$INTERNAL$Modal(props) {
  if (!props.isOpen) {
    return M$react_index$jsx('div', {
      className: "modal closed"
    }, null);
  }
  return M$react_index$jsx('div', {
    className: "modal open",
    children: [M$react_index$jsx('div', {
      className: "overlay",
      onClick: props.onClose,
      children: 'X'
    }, null), M$react_index$jsx('div', {
      className: "content",
      children: props.children
    }, null)]
  }, null);
}
function M$App$INTERNAL$Tooltip(props) {
  if (!props.isOpen) {
    return M$react_index$jsx('div', {
      className: "tooltip closed"
    }, null);
  }
  return M$react_index$jsx('div', {
    className: "tooltip open",
    children: [M$react_index$jsx('div', {
      className: "arrow"
    }, null), M$react_index$jsx('div', {
      className: "content",
      children: props.children
    }, null)]
  }, null);
}
function M$App$default(props) {
  var _M$react_index$useSta = M$react_index$useState(''),
    _M$react_index$useSta2 = _slicedToArray(_M$react_index$useSta, 2),
    text = _M$react_index$useSta2[0],
    setText = _M$react_index$useSta2[1];
  var _M$react_index$useSta3 = M$react_index$useState(0),
    _M$react_index$useSta4 = _slicedToArray(_M$react_index$useSta3, 2),
    number = _M$react_index$useSta4[0],
    setNumber = _M$react_index$useSta4[1];
  var _M$react_index$useSta5 = M$react_index$useState(false),
    _M$react_index$useSta6 = _slicedToArray(_M$react_index$useSta5, 2),
    isChecked = _M$react_index$useSta6[0],
    setIsChecked = _M$react_index$useSta6[1];
  var _M$react_index$useSta7 = M$react_index$useState(false),
    _M$react_index$useSta8 = _slicedToArray(_M$react_index$useSta7, 2),
    isSelected = _M$react_index$useSta8[0],
    setIsSelected = _M$react_index$useSta8[1];
  var _M$react_index$useSta9 = M$react_index$useState(false),
    _M$react_index$useSta10 = _slicedToArray(_M$react_index$useSta9, 2),
    isOpen = _M$react_index$useSta10[0],
    setIsOpen = _M$react_index$useSta10[1];
  var _M$react_index$useSta11 = M$react_index$useState(true),
    _M$react_index$useSta12 = _slicedToArray(_M$react_index$useSta11, 2),
    isTooltipOpen = _M$react_index$useSta12[0],
    setIsTooltipOpen = _M$react_index$useSta12[1];
  return M$react_index$jsx('div', {
    children: [M$react_index$jsx('h1', {
      children: 'React Benchmark'
    }, null), M$react_index$jsx(M$App$INTERNAL$Button, {
      id: "toggle-modal",
      onClick: function onClick() {
        return setIsOpen(!isOpen);
      },
      children: 'Toggle Modal'
    }, null), M$react_index$jsx(M$App$INTERNAL$Modal, {
      isOpen: isOpen,
      onClose: function onClose() {
        return setIsOpen(false);
      },
      children: [M$react_index$jsx('h2', {
        children: 'Modal Content'
      }, null), M$react_index$jsx('p', {
        children: 'This is some modal content.'
      }, null), M$react_index$jsx(M$App$INTERNAL$Tooltip, {
        isOpen: isTooltipOpen,
        onClose: function onClose() {
          return setIsTooltipOpen(false);
        },
        children: [M$react_index$jsx('h3', {
          children: 'Tooltip Content'
        }, null), M$react_index$jsx('p', {
          children: 'This is some tooltip content.'
        }, null)]
      }, null)]
    }, null), M$react_index$jsx('div', {
      children: [M$react_index$jsx('h2', {
        children: 'Form Elements'
      }, null), M$react_index$jsx(M$App$INTERNAL$Input, {
        id: "update-text",
        value: text,
        onChange: function onChange(e) {
          return setText(e.target.value);
        }
      }, null), M$react_index$jsx(M$App$INTERNAL$TextArea, {
        value: text,
        onChange: function onChange(e) {
          return setText(e.target.value);
        }
      }, null), M$react_index$jsx(M$App$INTERNAL$Select, {
        options: [{
          label: 'Option 1',
          value: 1
        }, {
          label: 'Option 2',
          value: 2
        }, {
          label: 'Option 3',
          value: 3
        }],
        onChange: function onChange(e) {
          return setNumber(parseInt(e.target.value));
        }
      }, null), M$react_index$jsx(M$App$INTERNAL$Checkbox, {
        checked: isChecked,
        onChange: function onChange(e) {
          return setIsChecked(e.target.checked);
        }
      }, null), M$react_index$jsx(M$App$INTERNAL$Radio, {
        checked: isSelected,
        onChange: function onChange(e) {
          return setIsSelected(e.target.checked);
        }
      }, null), M$react_index$jsx(M$App$INTERNAL$Slider, {
        min: 0,
        max: 100,
        step: 1,
        value: number,
        onChange: function onChange(e) {
          return setNumber(parseInt(e.target.value));
        }
      }, null), M$react_index$jsx(M$App$INTERNAL$ProgressBar, {
        progress: number
      }, null), M$react_index$jsx(M$App$INTERNAL$Spinner, {}, null)]
    }, null)]
  }, null);
}
/* file: app/simple/index.js */
function M$index$INTERNAL$printIf1(i, str) {
  if (i === 1) {
    print('===============================');
    print(str);
    print('===============================');
  }
}
function M$index$INTERNAL$run(N) {
  // Warmup
  for (var i = 1; i <= 100; ++i) {
    var root = M$react_index$createRoot();
    var rootElement = M$react_index$jsx(M$App$default, {}, null);
    M$index$INTERNAL$printIf1(i, root.render(rootElement));
    M$react_index$callOnClickOrChange('toggle-modal', null);
    M$react_index$callOnClickOrChange('update-text', {
      target: {
        value: '!!!!! some text !!!!!'
      }
    });
    M$sh_microtask$drainMicrotaskQueue();
    M$index$INTERNAL$printIf1(i, root.render(rootElement));
  }
  // Benchmark
  var start = Date.now();
  for (var _i = 1; _i <= N; ++_i) {
    var _root = M$react_index$createRoot();
    var _rootElement = M$react_index$jsx(M$App$default, {}, null);
    _root.render(_rootElement);
    M$react_index$callOnClickOrChange('toggle-modal', null);
    M$react_index$callOnClickOrChange('update-text', {
      target: {
        value: '!!!!! some text !!!!!'
      }
    });
    M$sh_microtask$drainMicrotaskQueue();
    _root.render(_rootElement);
  }
  var end = Date.now();
  print(`${end - start} ms`);
}
M$index$INTERNAL$run(10000);
//# sourceMappingURL=simple-es5.js.map
