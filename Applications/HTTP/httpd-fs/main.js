var templates = {};
templates['status'] = `
        <h3>FreeRTOS</h3>
        <table class="table table-striped">
          <tr>
            <td><b>Uptime:</b></td>
            <td>{{uptime}}</td>
          </tr>
          <tr>
            <td><b>Free heap:</b></td>
            <td>{{free_heap}}</td>
          </tr>
          <tr>
            <td><b>LED state:</b></td>
            <td>
              <div class="onoffswitch">
                <input type="checkbox" name="led" class="onoffswitch-checkbox" id="led-switch" {{#led}}checked{{/led}}>
                <label class="onoffswitch-label" for="led-switch">
                  <span class="onoffswitch-inner"></span>
                  <span class="onoffswitch-switch"></span>
                </label>
              </div>
            </td>
          </tr>
        </table>
`;
templates['config'] = `
        <h3>Clickboards</h3>
        <table class="table table-striped">
          <tr>
            <th>Clickboard</td>
            <th>Port 1</td>
            <th>Port 2</td>
          </tr>
          <tr>
              <td>None</td>
              <td><input type="radio" name="port1" value="none" checked="checked" onchange="configSubmit(this)"></td>
              <td><input type="radio" name="port2" value="none" checked="checked" onchange="configSubmit(this)"></td>
          {{#clickboards}}
          <tr>
            <td>{{name_format}}</td>
            <td><input type="radio" name="port1" value="{{name}}" {{#port1_active}}checked="checked"{{/port1_active}} {{^port1_available}}disabled="disabled"{{/port1_available}}></td>
            <td><input type="radio" name="port2" value="{{name}}" {{#port2_active}}checked="checked"{{/port2_active}} {{^port2_available}}disabled="disabled"{{/port2_available}}></td>
          </tr>
          {{/clickboards}}
        </table>
`;
templates['color2'] = `
        <h3>Sensor</h3>
        <table class="table table-striped">
          <tr>
            <th>Color</th>
            <th>Raw</th>
            <th>Normalized</th>
          </tr>
          <tr>
            <td>Red</td>
            <td>{{r}}</td>
            <td>{{r_norm}}</td>
          </tr>
          <tr>
            <td>Green</td>
            <td>{{g}}</td>
            <td>{{g_norm}}</td>
          </tr>
          <tr>
            <td>Blue</td>
            <td>{{b}}</td>
            <td>{{b_norm}}</td>
          </tr>
        </table>

        <h3>Color</h3>
        <table class="table table-striped">
            <tr>
                <td><b>Hex:</b></td>
                <td>#{{r_hex}}{{g_hex}}{{b_hex}}</td>
            </tr>
        </table>
        <div style="width:200px; height:100px; margin:auto; background:rgb({{r_dec}},{{g_dec}},{{b_dec}});"></div>
`;
templates['thermo3'] = `
        <h3>Temperature</h3>
        <table class="table table-striped">
            <tr>
                <td>Current temperature</td>
                <td>{{cur}}&deg;C</td>
            </tr>
            <tr>
                <td>Highest temperature&nbsp;{{highTime}}&nbsp;seconds ago</td>
                <td>{{high}}&deg;C</td>
            </tr>
            <tr>
                <td>Lowest temperature&nbsp;{{lowTime}}&nbsp;seconds ago</td>
                <td>{{low}}&deg;C</td>
            </tr>
        </table>
        <h3>History</h3>
        <table class="table table-striped">
            {{#history}}
            <tr>
                <td>{{date}}</td>
                <td>{{val}}&deg;C</td>
            </tr>
            {{/history}}
        </table>
    <input type="button" id="resetTemp" value="Reset" onclick="resetTempHist()">
`;
templates['expand2'] = `
        <h3>Water Meter</h3>
        <table class="table table-striped">
            {{#watermeter}}
            <tr>
                <td>Water Meter {{name}}</td>
                <td>{{quantity}}&nbsp;Liter</td>
                <td>
                    <select name="pin{{index}}">
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
                <td><input type="number" name="multi" min="0" step="10" value="{{multi}}"> ml</td>
            </tr>
        </table>
        <h3>Input Register</h3>
        <table class="table table-striped">
            <tr>
                <td>Value</td>
                <td>{{input}}</td>
            </tr>
            <tr>
                <td>Bits</td>
                <td class="bits">
                    {{#inputs}}
                    <label><input type="checkbox" disabled name="{{name}}" {{#val}}checked{{/val}}>{{number}}</label>
                    {{/inputs}}
                </td>
            </tr>
        </table>
        <h3>Output Register</h3>
        <table class="table table-striped">
            <tr>
                <td>Value</td>
                <td><input type="number" name="output" min="0" max="255" step="1" value="{{output}}"></td>
            </tr>
            <tr>
                <td>Bits</td>
                <td class="bits">
                    {{#outputs}}
                    <label><input type="checkbox" name="output[]" onchange="toggleBit(this, event)" {{#val}}checked{{/val}}>{{number}}</label>
                    {{/outputs}}
                </td>
            </tr>
        </table>
`;

function toggleBit( element, event ) {
    var x = 0;
    $($('input[name="'+element.name+'"').get().reverse()).each(function(i, v) {
        console.log(v);
        $(v).prop('checked') && ( x += 2**i );
    });
    $('input[name="'+element.name.substring(0, element.name.lastIndexOf('['))+'"]').val(x).change();

    event.stopPropagation();
}

function capitalize(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

// thermo3click variables
var tempHistory = [];
// expand2click variables
var wmeterPin = [0,0];
var wmeterMultiplicator = 0.25;

function processJSON(page, json) {
    switch(page) {
        case 'status':
            $("#hostname").text(json['hostname']);
            break;
        case 'config':
            $('#nav .clickboard').addClass('hidden');
            $.each(json['clickboards'], function(i, clickboard) {
                clickboard['name_format'] = capitalize(clickboard['name']) + 'Click';
                clickboard['port1_available'] = clickboard['available'] & (1 << 0) ? true : false;
                clickboard['port2_available'] = clickboard['available'] & (1 << 1) ? true : false;
                clickboard['port1_active'] = clickboard['active'] & (1 << 0) ? true : false;
                clickboard['port2_active'] = clickboard['active'] & (1 << 1) ? true : false;

                /* If the clickboard is active, add an entry to the menu. */
                if(clickboard['port1_active']) {
                    $('#nav li.clickboard').eq(0).removeClass('hidden').find('a').attr('href', '#'+clickboard['name']).find('span').text(clickboard['name_format']);
                }
                if(clickboard['port2_active']) {
                    $('#nav li.clickboard').eq(1).removeClass('hidden').find('a').attr('href', '#'+clickboard['name']).find('span').text(clickboard['name_format']);
                }
            });
            break;
        case 'color2':
            var rNorm = 1.0;
            var gNorm = 2.12;
            var bNorm = 2.0;
            json['r_norm'] = Math.round(json['r'] / rNorm);
            json['g_norm'] = Math.round(json['g'] / gNorm);
            json['b_norm'] = Math.round(json['b'] / bNorm);
            var colorMax = Math.max(json['r_norm'], json['g_norm'], json['b_norm']);
            json['r_dec'] = Math.round(json['r_norm'] * 255 / colorMax);
            json['g_dec'] = Math.round(json['g_norm'] * 255 / colorMax);
            json['b_dec'] = Math.round(json['b_norm'] * 255 / colorMax);
            json['r_hex'] = json['r_dec'].toString(16);
            json['g_hex'] = json['g_dec'].toString(16);
            json['b_hex'] = json['b_dec'].toString(16);
            break;
        case 'thermo3':
            json['cur'] = json['temp_cur'] / 100;
            json['high'] = json['temp_high'] / 100;
            json['low'] = json['temp_low'] / 100;
            // Store temp value in the global history
            tempHistory.unshift({ 'date' : new Date().toLocaleString('de-DE'), 'val' : json['cur'] });
            json['history'] = tempHistory;
            break;
        case 'expand2':
            json['inputs'] = [];
            json['outputs'] = [];
            for( var i = 0; i < 8; i++ ) {
                json['inputs'].unshift({'number': i, 'val': (json['input'] & ( 1 << i )) });
                json['outputs'].unshift({'number':i, 'val': (json['output'] & ( 1 << i )) });
            }
            // Convert toggle count to water quantity
            json['watermeter'] = [];
            for( var x = 0; x < 2; x++ ) {
                var options = [{ 'val': 0, 'sel': (!json['pin'+x] ? 1 : 0), 'name': 'Off' }];
                for( var i = 0; i < 8; i++) {
                    options.push({ 'val': Math.pow(2, i) , 'sel': (json['pin'+x] & ( 1 << i )), 'name': 'PA'+i});
                }
                json['watermeter'].push({
                    'index': x,
                    'name': x+1,
                    'quantity': (json['count'+x] * json['multi'] / 1000).toFixed(3),
                    'options': options
                });
            }
        default:
            break;
    }
    return json;
}

function sendRequest(page, data, success) {
    var domain = 'http://172.16.201.3/';
    if( !data ) data = { action: 'get' };
    $.getJSON(domain + page + '.json', data, success)
            .fail(function(xhr, text_status, error_thrown) {
                    // Retry after 3s, unless request was explicitly aborted
                    console.log(text_status);
                    console.log(error_thrown);
                    if( text_status != "abort" ) {
                        setTimeout( function() { sendRequest(page, data, success) }, 3000 );
                    }
            });
}

// Store the currently visible page
var currentPage;
function renderPage(page, json) {
    var html = Mustache.render(templates[page], processJSON(page, json));

    if( currentPage == page ) {
        $('#content').html(html);
    } else {
        currentPage = page;
//        $('#content').fadeOut(100, function() {
//            $('#content').html(html).fadeIn(200);
//        });
        $('#content').html(html)
        // Mark navigation link as active
        $('#nav a.active').removeClass('active');
        $('#nav a[href^="#' + page + '"]').addClass('active');

        // Change title
        $('#page-title').html($('#nav a.active').html());
    }
}

var timeout;
function updatePage(page, data) {
    if( timeout ) clearTimeout(timeout);

    if( !page ) {
      page = window.location.hash.substr(1);
    } else {
        window.location.hash = '#' + page;
    }
    if( !(page in templates) ) {
        page = 'status';
    }

    sendRequest(page, data, function(json) {
        renderPage(page, json);
        timeout = setTimeout(updatePage, 1000);
    });
}

// Handle internal links by JQuery
$(document).on('click', 'a[href^="#"]', function(event) {
    event.preventDefault();
    updatePage(this.hash.substr(1));
});

// Submit input fields on change
$(document).on('change', 'input, select', function() {
    console.log(this);
    console.log($(this));
    console.log($(this).serialize());
    updatePage(undefined, $(this).serialize());
});

// Stop auto refresh when focusing input fields
$(document).on('focus', 'input, select', function() {
    if( timeout ) clearTimeout(timeout);
});

// Refresh page when leaving input fields
$(document).on('focusout', 'input, select', function() {
    updatePage();
});

// Load config once to add current clickboards to menu
sendRequest('config', undefined, function(json) {
    processJSON('config', json);
});

// Initialize current page
updatePage();