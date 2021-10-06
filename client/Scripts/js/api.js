var api = {
    version: 204,

    actDevice: function(group, type, newState, user_id) {
        group.write_to_control(type, newState, api.type.mode.automatic, user_id)
    },
    findItem: function(items, func) {
        for (var item_idx in items)
        {
            var item = items[item_idx];
            if (func(item))
                return item;
        }
        return null;
    },
    status: {},
    type: {
        item: {}, group: {}, mode: { unknown: 0 }, param: {}
    },
    checker: [],

    handlers: {
        changed: {
            mode: undefined,
            item: undefined,
            sensor: undefined,
            control: undefined,
            day_part: undefined,
        },
        database: { initialized: undefined },
        section: { initialized: undefined },
        group: { initialized: {}, changed: {} }, // fill group names
        control_change_check: undefined,
        normalize: undefined,
        check_value: undefined,
        group_status: undefined,
        initialized: undefined,
        can_restart: undefined
    },
};

api.get_number = function(value, default_value)
{
    if (value !== undefined)
    {
        if (typeof value !== 'number')
            value = parseFloat(value.valueOf());
        if (value === value)
            return value;
    }
    return default_value;
};

api.get_number_from_property = function(item, default_value, prop_name)
{
    if (item && item.is_connected)
        return api.get_number(item[prop_name], default_value);
    return default_value;
};

api.get_number_value = function(item, default_value)
{
    return api.get_number_from_property(item, default_value, 'value');
};

api.get_number_raw_value = function(item, default_value)
{
    return api.get_number_from_property(item, default_value, 'raw_value');
};

api.set_default_param_value = function(param_or_group, value, param_name)
{
    var param = param_name ? param_or_group.param : param_or_group;
    if (param_name)
    {
        var name_list = param_name.split('.');
        for (var i in name_list)
        {
            var name = name_list[i];
            param = param[name];
            if (!param)
            {
                console.warm("api.set_default_param_value: unknown param_name " + param_name + " in group " + group);
                return;
            }
        }
    }

    if (!param.value)
        param.value = value;
};

api.extend = function(Child, Parent)
{
    var F = function() {};
    F.prototype = Parent.prototype;
    Child.prototype = new F();
    Child.prototype.constructor = Child;
    Child.superclass = Parent.prototype;
};

api.mixin = function(dst, src)
{
    var tobj = {};
    for(var x in src)
        if((typeof tobj[x] == "undefined") || (tobj[x] != src[x]))
            dst[x] = src[x];
};

api.get_type_name = function(type_id, types)
{
    for (var i in types)
        if (types[i] === type_id)
            return i;
    return '[unknown_type]';
};

api.connect_if_exist = function(signal, obj, func_name)
{
    if (typeof obj[func_name] === 'function')
        signal.connect(obj, obj[func_name]);
};

api.init_as_group_manager = function(obj, group)
{
    obj.group = group;
    obj.param = group.param;
    obj.item = {};

    var items = group.items;
    for (var i in items)
    {
        var item = items[i];
        var type_name = api.get_type_name(item.type, api.type.item);
        obj.item[type_name] = item;

        api.connect_if_exist(item.value_changed, obj, 'on_' + type_name); // args: user_id
        api.connect_if_exist(item.connection_state_changed, obj, 'on_' + type_name + '_connection_state'); // args: bool state

        var func = obj['on_' + type_name + '_raw_to_display'];
        if (typeof func === 'function')
            api.mng.connect_item_raw_to_display(item, obj, func); // args: data

        func = obj['on_' + type_name + '_display_to_raw'];
        if (typeof func === 'function')
            api.mng.connect_item_display_to_raw(item, obj, func); // args: data, user_id

        func = obj['on_' + type_name + '_clarify_connection_state'];
        if (typeof func === 'function')
            api.mng.connect_item_clarify_connection_state(item, obj, func); // args: state

        func = obj['is_' + type_name + '_can_change'];
        if (typeof func === 'function')
            api.mng.connect_item_is_can_change(item, obj, func); // args: raw_data, user_id
    }

    api.connect_if_exist(group.mode_changed , obj, 'on_mode_changed');  // args: user_id, mode_id
    api.connect_if_exist(group.item_changed , obj, 'on_item_changed');  // args: item, user_id
    api.connect_if_exist(group.param_changed, obj, 'on_param_changed'); // args: param, user_id

    if (typeof obj.on_item_change_check === 'function')
        api.mng.connect_group_is_can_change(group, obj, obj.on_item_change_check); // args: item, raw_data, user_id
};

api.props = function(obj)
{
    var p = [];
    for (; obj != null; obj = Object.getPrototypeOf(obj))
    {
        var op = Object.getOwnPropertyNames(obj);
        for (var i=0; i<op.length; i++)
            if (p.indexOf(op[i]) == -1)
                 p.push(op[i]);
    }
    return p;
};

var modbus = {

    DiscreteInputs: 1, DI: 1,
    Coils: 2, C: 2,
    InputRegisters: 3, IR: 3,
    HoldingRegisters: 4, HR:4,

    read: function(server, regType, address, count) {
        regType = regType !== undefined ? regType : 3;
        address = address !== undefined ? address : 0;
        count = count !== undefined ? count : 1;

        return api.mng.modbusRead(server, regType, address, count);
    },

    write: function(server, regType, unit, value) {
        api.mng.modbusWrite(server, regType, unit, value);
    },

    stop: function() { api.mng.modbusStop(); },
    start: function() { api.mng.modbusStart(); },
}

var console = {
    'log': function(text, user_id, inform, print_backtrace) { api.mng.log(text, 0, user_id, inform, print_backtrace) },
    'info': function(text, user_id, inform, print_backtrace) { api.mng.log(text, 4, user_id, inform, print_backtrace) },
    'warn': function(text, user_id, inform, print_backtrace) { api.mng.log(text, 1, user_id, inform, print_backtrace) },
    'critical': function(text, user_id, inform, print_backtrace) { api.mng.log(text, 2, user_id, inform, print_backtrace) },
    'error': function(text, user_id, inform, print_backtrace) { api.mng.log(text, 2, user_id, inform, print_backtrace) },
    'err': function(text, user_id, inform, print_backtrace) { api.mng.log(text, 2, user_id, inform, print_backtrace) },
}

console.warning = console.warn;

function checkVar(state) {
    return typeof state != 'undefined'
}
