var template = {
    page: '',
    template: ``,     /* html template with mustache variables */
    templateData: {}, /* data passed to mustache */
    cache: null,      /* last received json response */
    view: null,       /* detached view */
    defaultRender: function( key, value ) {
        var el = this.view.find('#'+this.page+'-'+key).first();
        if( el != null ) {
            if( el.is('input') ) {
                if( el.is('[type="checkbox"]') ) {
                    el.prop('checked', value);
                }
                else {
                    el.val(value);
                }
            } else {
                el.text(value);
            }
        }
    },
    init: function() {
        this.view = $(Mustache.render(this.template, this.templateData));
    },
    filter: function( data ) {
        // Store response in cache
        if( this.cache == null ) {
            this.cache = $.extend(true, {}, data); /* deep copy */
        } else {
            for( key in data ) {
                if( equals(this.cache[key], data[key] )) {
                    // Filter out unchanged values
                    delete data[key];
                } else {
                    // Update changes in cache
                    if( data[key] instanceof Object ) {
                        this.cache[key] = $.extend(true, {}, data[key]); /* deep copy */
                    } else {
                        this.cache[key] = data[key] /* primitive type */
                    }
                }
            }
        }
    },
    parse: function( cache, data ) {
    },
    render: function( data) {
        for( key in data ) {
            this.defaultRender( key, data[key] );
        }
    },
    update: function( data ) {
        this.filter( data ); /* modifies data to only contain values that changed and updates the cache */
        this.parse( data );
        this.render( data );
    },
    show: function() {
        if( this.view == null ) {
            this.init();
        }
        $('#content').append(this.view);
    },
    hide: function() {
        if( this.view != null ) this.view.detach();
    }
};

var templates = {};
templates['status'] = Object.assign(Object.create(template), {
    page: 'status',
    template: `
        <h3>System</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td>Uptime</td>
            <td><span id="status-uptime"></span>&nbsp;s</td>
          </tr>
          <tr>
            <td>Free heap</td>
            <td><span id="status-free_heap"></span>&nbsp;B</td>
          </tr>
          <tr>
            <td>Build</td>
            <td id="status-build"></td>
          </tr>
        </table>
        <h3>EvalBoard</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>USR LED</b></td>
            <td>
              <div class="onoffswitch">
                <input type="checkbox" name="led" class="onoffswitch-checkbox" id="status-led" {{#led}}checked{{/led}}>
                <label class="onoffswitch-label" for="status-led">
                  <span class="onoffswitch-inner"></span>
                  <span class="onoffswitch-switch"></span>
                </label>
              </div>
            </td>
          </tr>
          <tr>
            <td><b>MINT Button</b></td>
            <td>
              <div class="onoffswitch">
                <input type="checkbox" name="button" class="onoffswitch-checkbox" id="status-button" disabled="disabled" {{#button}}checked{{/button}}>
                <label class="onoffswitch-label" for="status-button">
                  <span class="onoffswitch-inner"></span>
                  <span class="onoffswitch-switch"></span>
                </label>
              </div>
            </td>
          </tr>
        </table>
        <h3>Network</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>Hostname</b></td>
            <td id="status-hostname">{{hostname}}</td>
          </tr>
          <tr>
            <td><b>MAC</b></td>
            <td id="status-mac">{{mac}}</td>
          </tr>
          <tr>
            <td><b>IP</b></td>
            <td id="status-ip">{{ip}}</td>
          </tr>
          <tr>
            <td><b>Netmask</b></td>
            <td id="status-netmask">{{netmask}}</td>
          </tr>
          <tr>
            <td><b>Gateway</b></td>
            <td id="status-gateway">{{gateway}}</td>
          </tr>
          <tr>
            <td><b>DNS</b></td>
            <td id="status-dns">{{dns}}</td>
          </tr>
        </table>
`,
    render: function( data ) {
        Object.getPrototypeOf( this ).render.call( this, data ); /* call parent */
        if( 'hostname' in data ) this.updateHostname( data['hostname']);
    },
    updateHostname: function( hostname ) {
        $('#hostname').text( hostname );
    }
});

templates['config'] = Object.assign(Object.create(template), {
    page: 'config',
    template: `
        <h3>Clickboards</h3>
        <form id="config-clickboards-form">
          <table class="mui-table mui-table--bordered" id="config-clickboards">
            <tr>
              <th>Clickboard</th>
              <th>Port 1</th>
              <th>Port 2</th>
            </tr>
            <tr id="config-clickboard-none">
                <td>None</td>
                <td><input type="radio" name="port1" value="none" checked="checked"></td>
                <td><input type="radio" name="port2" value="none" checked="checked"></td>
            </tr>
          </table>
          <input type="submit">
        </form>
`,
    parse: function( data ) {
        if( 'clickboards' in data ) {
            for( i in data.clickboards ) {
                var clickboard = data.clickboards[i];
                clickboard.name_format = capitalize(clickboard.name) + 'Click';
            }
        }
    },
    render: function( data )  {
        for( key in data ) {
            if( key == 'clickboards' ) {
                this.updateMenu(data.clickboards);

                // remove outdated clickboard config rows
                this.view.find('.config-clickboard').remove();
                var active = 0;
                for( i in data.clickboards ) {
                    var clickboard = data.clickboards[i];
                    active += clickboard.active;

                    // add row to clickboard config
                    var tr = $('<tr/>', {id: 'config-clickboard-' + clickboard.name, 'class': 'config-clickboard'} );
                    tr.append( '<td>' + clickboard.name_format + '</td>' );

                    for( var port = 0; port < 2; ++port ) {
                        var checked  = ( ( clickboard.active    & (1 << port) ) != 0 ) ? 'checked="checked" '   : '';
                        var disabled = ( ( clickboard.available & (1 << port) ) == 0 ) ? 'disabled="disabled" ' : '';
                        tr.append( '<td><input type="radio" name="port' + (port+1) + '" value="'+ clickboard.name + '"' + checked + disabled + '></td>' );
                    }

                    this.view.find('#config-clickboards').append(tr);
                }
                for( var port = 0; port < 2; ++port ) {
                    $('#config-clickboard-none [name=port'+(port+1)+']').prop('checked', (active & (1 << port)) == 0);
                }
            } else {
                this.defaultRender( key, data[key] );
            }
        }
    },
    updateMenu: function( clickboards ) {
        // hide menu entries
        $('#nav .clickboard').addClass('hidden');
        for( i in clickboards ) {
            var clickboard = clickboards[i];
            clickboard.name_format = capitalize(clickboard.name) + 'Click';
            for( var port = 0; port < 2; ++port ) {
                // update and show menu
                if( ( clickboard['active'] & (1 << port) ) != 0 ) {
                    $('#nav li.clickboard').eq(port).removeClass('hidden').find('a').attr('href', '#'+clickboard.name).find('span').text(clickboard.name_format);
                }
            }
        }
    }
});

templates['color2'] = Object.assign(Object.create(template), {
    page: 'color2',
    template: `
        <h3>Sensor</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <th>Color</th>
            <th>Raw</th>
            <th>Normalized</th>
          </tr>
          <tr>
            <td>Red</td>
            <td id="color2-r">{{r}}</td>
            <td id="color2-r_norm">{{r_norm}}</td>
          </tr>
          <tr>
            <td>Green</td>
            <td id="color2-g">{{g}}</td>
            <td id="color2-g_norm">{{g_norm}}</td>
          </tr>
          <tr>
            <td>Blue</td>
            <td id="color2-b">{{b}}</td>
            <td id="color2-b_norm">{{b_norm}}</td>
          </tr>
        </table>

        <h3>Color</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td><b>Hex</b></td>
                <td id="color2-rgb_hex">#{{r_hex}}{{g_hex}}{{b_hex}}</td>
            </tr>
        </table>
        <div id="color2-color-box" style="width:200px; height:100px; margin:auto; background:rgb({{r_dec}},{{g_dec}},{{b_dec}});"></div>
`,
    parse: function( data ) {
        if( ('r' in data) || ('g' in data) || ('b' in data) ) {
            var rNorm = 1.0;
            var gNorm = 2.12;
            var bNorm = 2.0;
            data['r_norm'] = Math.round(this.cache['r'] / rNorm);
            data['g_norm'] = Math.round(this.cache['g'] / gNorm);
            data['b_norm'] = Math.round(this.cache['b'] / bNorm);
            var colorMax = Math.max(data['r_norm'], data['g_norm'], data['b_norm']);
            data['rgb_hex'] = '#' + Math.round(data['r_norm'] * 255 / colorMax).toString(16)
                            + Math.round(data['g_norm'] * 255 / colorMax).toString(16)
                            + Math.round(data['b_norm'] * 255 / colorMax).toString(16);
        }
    },
    render: function( data ) {
        Object.getPrototypeOf( this ).render.call( this, data ); /* call parent */
        if( 'rgb_hex' in data ) $('#'+this.page+'-color-box').css('background',data['rgb_hex']);
    }
});

templates['thermo3'] = Object.assign(Object.create(template), {
    page: 'thermo3',
    template: `
        <h3>Temperature</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Current temperature</td>
                <td><span id="thermo3-temp_cur"></span>&nbsp;&deg;C</td>
            </tr>
            <tr>
                <td>Highest temperature&thinsp;<span id="thermo3-temp_high_time">0</span>&nbsp;s ago</td>
                <td><span id="thermo3-temp_high"></span>&nbsp;&deg;C</td>
            </tr>
            <tr>
                <td>Lowest temperature&thinsp;<span id="thermo3-temp_low_time">0</span>&nbsp;s ago</td>
                <td><span id="thermo3-temp_low"></span>&nbsp;&deg;C</td>
            </tr>
        </table>
        <h3>History</h3>
        <table class="mui-table mui-table--bordered" id="thermo3-log">
            <tbody>
            </tbody>
        </table>
    <input type="button" id="thermo3-resetLog" value="Reset" onclick="templates['thermo3'].resetLog()">
`,
    parse: function( data ) {
        for( key in data ) {
            switch( key ) {
            case 'temp_cur':
            case 'temp_high':
            case 'temp_low':
                data[key] = (data[key]/100).toFixed(2);
                break;
            }            
        }

        data['log'] = { 'date' : new Date().toLocaleString('de-DE'), 'val' : (this.cache['temp_cur'] / 100) };
    },
    render: function( data ) {
        Object.getPrototypeOf( this ).render.call( this, data ); /* call parent */
        if( 'log' in data ) {
            $('#'+this.page+'-log tbody').prepend('<tr><td>'+data.log.date+'</td><td>'+data.log.val+' °C</td></tr>');
        }
    },
    resetLog: function() {
      $('#'+this.page+'-log tbody').empty();
    }
});

templates['expand2'] = Object.assign(Object.create(template), {
    page: 'expand2',
    template: `
        <h3>Water Meter</h3>
        <table class="mui-table mui-table--bordered">
            {{#watermeter}}
            <tr>
                <td>Water Meter {{name}}</td>
                <td>{{quantity}}&nbsp;Liter</td>
                <td>
                    <select id="expand2-pin{{index}}" name="pin{{index}}">
                        {{#options}}
                        <option value="{{val}}" {{#sel}}selected{{/sel}}>{{name}}</option>
                        {{/options}}
                    </select>
                </td>
            </tr>
            {{/watermeter}}
            <tr>
                <td>Multiplicator</td>
                <td></td>
                <td><input type="number" name="multi" min="0" step="10" value="{{multi}}">&nbsp;ml</td>
            </tr>
        </table>
        <h3>Input Register</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Value</td>
                <td><input id="expand2-input" type="number" name="input" min="0" max="255" step="1" disabled></td>
            </tr>
            <tr>
                <td>Bits</td>
                <td class="bits">
                    {{#inputs}}
                    <label><input id="expand2-input{{number}}" type="checkbox" name="input[]" disabled>{{number}}</label>
                    {{/inputs}}
                </td>
            </tr>
        </table>
        <h3>Output Register</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Value</td>
                <td><input id="expand2-output" type="number" name="output" min="0" max="255" step="1"></td>
            </tr>
            <tr>
                <td>Bits</td>
                <td class="bits">
                    {{#outputs}}
                    <label><input id="expand2-output{{number}}" type="checkbox" name="output[]" onchange="templates['expand2'].toggleBit();event.stopPropagation();">{{number}}</label>
                    {{/outputs}}
                </td>
            </tr>
        </table>
`,
    templateData: (function() {
        data = {};
        data['watermeter'] = [];
        for( var x = 0; x < 2; x++ ) {
            var options = [{ 'val': 0, 'sel': true, 'name': 'Off' }];
            for( var i = 0; i < 8; i++) {
                options.push({ 'val': 1<<i, 'sel': false, 'name': 'PA'+i});
            }
            data['watermeter'].push({
                'index': x,
                'name': x+1,
                'quantity': 0,
                'options': options
            });
        }
        data['inputs'] = [];
        data['outputs'] = [];
        for( var i = 0; i < 8; i++ ) {
            data['inputs'].unshift({'number': i, 'val': false });
            data['outputs'].unshift({'number':i, 'val': false });
        }
        return data;
    })(),
    parse: function( data ) {
        for( var x = 0; x < 2; x++ ) {
            if( ('count'+x) in data )
                data['count'+x] = (data['count'+x] * this.cache['multi'] / 1000).toFixed(3);
        }
    },
    render: function( data ) {
        for( key in data ) {
            switch( key ) {
            case 'pin0':
            case 'pin1':
                this.view.find('#'+this.page+'-'+key+' option').attr('selected', false);
                this.view.find('#'+this.page+'-'+key+' option[value='+data[key]+']').attr('selected', true);
                break;
            case 'input':
            case 'output':
                this.view.find('#'+this.page+'-'+key).val(data[key]);
                for( var i = 0; i < 8; i++) {
                    this.view.find('#'+this.page+'-'+key+i).prop('checked', (data[key] & ( 1 << i )) != 0);
                }
                break;
            }
        }
    },
    toggleBit: function() {
        var x = 0;
        $(this.view.find('input[name="output[]"]').get().reverse()).each(function(i, v) {
            $(v).prop('checked') && ( x += 1<<i );
        });
        this.view.find('input[name="output"]').val(x).change();
    }
});

var site = {
    domain: '', /* 'http://172.16.200.127/' */
    maxRetries: 1,
    xhr: null,
    timeout: null,
    currentPage: null,
    request: function( page = this.currentPage, success = $.noop,
                       data = { action: 'get' }, tries = this.maxRetries ) {
        var that = this;
        var xhr = $.getJSON(this.domain + page + '.json', data)
          .done( function(json) {
            templates[page].update( json );
            success( page, json );
          })
          .fail( function(xhr, text_status, error_thrown) {
            if (text_status != "abort") {
                if( tries > 0 ) {
                    that.request( page, success, undefined, tries-1 );
                } else {
                    that.disconnected();
                }
            }
          });
        return xhr;
    },
    cancelUpdate: function() {
        if( this.xhr != null ) {
            this.xhr.abort();
            this.xhr = null;
        }
        if( this.timeout != null ) {
            clearTimeout( this.timeout );
            this.timeout = null;
        }
    },
    update: function( page = this.currentPage, delay = (getRefreshRate() * 1000), update = 0 ) {
        var that = this;
        this.cancelUpdate();
        this.timeout = setTimeout( function(){
            var updateFunction = undefined;
            if( !isNaN(delay) ) {
                updateFunction = function( page, json ) {
                    that.update( undefined, undefined, update+1 );
                };
            }
            that.xhr = that.request( page, updateFunction );
        }, delay );
    },
    disconnected: function() {
        $('#warning').addClass('active');
    },
    switch: function( page ) {
        if( !page ) {
            page = window.location.hash.substr(1);
        }
        if( !( page in templates ) ) {
            page = 'status';
        }
        if( page != this.currentPage ) {
            window.location.hash = '#' + page;
            if( this.currentPage in templates ) {
                templates[this.currentPage].hide();
            }
            this.currentPage = page;
            templates[ page ].show();

            // Mark navigation link as active
            $( '#nav a.active' ).removeClass( 'active' );
            $( '#nav a[href^="#' + page + '"]' ).addClass( 'active' );

            // Change title
            $( '#page-title' ).html( $( '#nav a.active' ).html() );
        }

        // immediately update
        this.update( undefined, 0 );        
    },
    init: function() {
        var that = this;
        for( page in templates ) {
          templates[page].init();
        }
        this.request( 'config' );
        this.request( 'status', function( page, json) {
            that.switch();
        })
    }
};

site.init();

/**
 * Deep compare of two objects.
 *
 * Note that this does not detect cyclical objects as it should.
 * Need to implement that when this is used in a more general case. It's currently only used
 * in a place that guarantees no cyclical structures.
 *
 * @param {*} x
 * @param {*} y
 * @return {Boolean} Whether the two objects are equivalent, that is,
 *         every property in x is equal to every property in y recursively. Primitives
 *         must be strictly equal, that is "1" and 1, null an undefined and similar objects
 *         are considered different
 *         y can be a subset of x.
 */
function equals ( x, y ) {
    // If both x and y are null or undefined and exactly the same
    if ( x === y ) {
        return true;
    }

    // If they are not strictly equal, they both need to be Objects
    if ( ! ( x instanceof Object ) || ! ( y instanceof Object ) ) {
        return false;
    }

    // They must have the exact same prototype chain, the closest we can do is
    // test the constructor.
    if ( x.constructor !== y.constructor ) {
        return false;
    }

    for ( var p in x ) {
        // Inherited properties were tested using x.constructor === y.constructor
        if ( x.hasOwnProperty( p ) ) {
            // Allows comparing x[ p ] and y[ p ] when set to undefined
            if ( ! y.hasOwnProperty( p ) ) {
                continue; /* continue if y is subset of x */
            }

            // If they have the same strict value or identity then they are equal
            if ( x[ p ] === y[ p ] ) {
                continue;
            }

            // Numbers, Strings, Functions, Booleans must be strictly equal
            if ( typeof( x[ p ] ) !== "object" ) {
                return false;
            }

            // Objects and Arrays must be tested recursively
            if ( !equals( x[ p ],  y[ p ] ) ) {
                return false;
            }
        }
    }

    for ( p in y ) {
        // allows x[ p ] to be set to undefined
        if ( y.hasOwnProperty( p ) && ! x.hasOwnProperty( p ) ) {
            return false;
        }
    }
    return true;
}

function capitalize(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

function setRefreshRate( rate ) {
    $('input[name="refresh"]').val(rate).trigger('input');
}

function getRefreshRate() {
    var rates = [0.5, 1, 3, 5, 10, 20, 30, 60];
    return rates[$('input[name="refresh"]').val()];
}

function serialize(element) {
    var data = $(element).serialize();
    if( $(element).is(':checkbox') && !element.checked ) {
        data += element.name + '=off';
    }
    return data;
}

// Handle internal links by JQuery
$(document).on('click', 'a[href^="#"]', function(event) {
    event.preventDefault();
    site.switch(this.hash.substr(1));
});

// Submit input fields on change
$(document).on('change', 'input, select', function() {
    site.cancelUpdate();
    site.request(undefined, function() { site.update() }, serialize(this));
});

// Button to open/close top right menu
$("#menu button").click(function(e) {
    $("#menu ul").toggle();
    e.stopPropagation();
});

// Close menu when clicked outside
$(document).click(function(){
  $("#menu ul").hide();
});

// Send request to write config to flash
$('#write-config').click(function() {
    site.request('config', undefined, 'write' );
});

// Send request to erase config from flash
$('#erase-config').click(function() {
    site.request( 'config', undefined, 'erase' );
});

// Send request to reset system
$('#reset-system').click(function() {
    site.request('status.json', undefined, 'reset', site.maxRetries ).abort();
    site.disconnected();
});

$('#warning button').click(function() {
    site.switch();
    $('#warning').removeClass('active');
});