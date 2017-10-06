var templates = {};
templates['status'] = `
        <h3>System</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>Uptime</b></td>
            <td>{{uptime}}</td>
          </tr>
          <tr>
            <td><b>Free heap</b></td>
            <td>{{free_heap}}</td>
          </tr>
          <tr>
            <td><b>Build</b></td>
            <td>{{build}}</td>
          </tr>
        </table>
        <h3>EvalBoard</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <td><b>USR LED</b></td>
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
          <tr>
            <td><b>MINT Button</b></td>
            <td>
              <div class="onoffswitch">
                <input type="checkbox" name="button" class="onoffswitch-checkbox" id="button-switch" disabled="disabled" {{#button}}checked{{/button}}>
                <label class="onoffswitch-label" for="button-switch">
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
            <td>{{hostname}}</td>
          </tr>
          <tr>
            <td><b>MAC</b></td>
            <td>{{mac}}</td>
          </tr>
          <tr>
            <td><b>IP</b></td>
            <td>{{ip}}</td>
          </tr>
          <tr>
            <td><b>Netmask</b></td>
            <td>{{netmask}}</td>
          </tr>
          <tr>
            <td><b>Gateway</b></td>
            <td>{{gateway}}</td>
          </tr>
          <tr>
            <td><b>DNS</b></td>
            <td>{{dns}}</td>
          </tr>
        </table>
`;
templates['config'] = `
        <h3>Clickboards</h3>
        <table class="mui-table mui-table--bordered">
          <tr>
            <th>Clickboard</td>
            <th>Port 1</td>
            <th>Port 2</td>
          </tr>
          <tr>
              <td>None</td>
              <td><input type="radio" name="port1" value="none" checked="checked"></td>
              <td><input type="radio" name="port2" value="none" checked="checked"></td>
          {{#clickboards}}
          <tr>
            <td>{{name_format}}</td>
            <td><input type="radio" name="port1" value="{{name}}" {{#port1_active}}checked="checked"{{/port1_active}} {{^port1_available}}disabled="disabled"{{/port1_available}}></td>
            <td><input type="radio" name="port2" value="{{name}}" {{#port2_active}}checked="checked"{{/port2_active}} {{^port2_available}}disabled="disabled"{{/port2_available}}></td>
          </tr>
          {{/clickboards}}
        </table>
		<h3>MQTT Client</h3>
		<table class="table table-striped">
			<tr>
				<td>Status</td>
				<div class="onoffswitch">
					<input type="checkbox" name="mqttActive" class="onoffswitch-checkbox" id="mqttActive" {{#mqtt}}checked{{/mqtt}}>
					<label class="onoffswitch-label" for="mqttActive">
						<span class="onoffswitch-inner"></span>
						<span class="onoffswitch-switch"></span>
					</label>
              </div>
			</tr>
		</table>
`;
templates['color2'] = `
        <h3>Sensor</h3>
        <table class="mui-table mui-table--bordered">
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
        <table class="mui-table mui-table--bordered">
            <tr>
                <td><b>Hex</b></td>
                <td>#{{r_hex}}{{g_hex}}{{b_hex}}</td>
            </tr>
        </table>
        <div style="width:200px; height:100px; margin:auto; background:rgb({{r_dec}},{{g_dec}},{{b_dec}});"></div>
`;
templates['thermo3'] = `
        <h3>Temperature</h3>
        <table class="mui-table mui-table--bordered">
            <tr>
                <td>Current temperature</td>
                <td>{{cur}}&deg;C</td>
            </tr>
            <tr>
                <td>Highest temperature&nbsp;{{temp_high_time}}&nbsp;seconds ago</td>
                <td>{{high}}&deg;C</td>
            </tr>
            <tr>
                <td>Lowest temperature&nbsp;{{temp_low_time}}&nbsp;seconds ago</td>
                <td>{{low}}&deg;C</td>
            </tr>
        </table>
        <h3>History</h3>
        <table class="mui-table mui-table--bordered">
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
        <table class="mui-table mui-table--bordered">
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
                <td><input type="number" name="multi" min="0" step="10" value="{{multi}}">&nbsp;ml</td>
            </tr>
        </table>
        <h3>Input Register</h3>
        <table class="mui-table mui-table--bordered">
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
        <table class="mui-table mui-table--bordered">
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
templates['mqtt'] = `
            <h3>MQTT Client Information</h3>
			<h4>Statistics</h4>
			<ul id="mqttstatus">
				<li>Online</li>
				<li>Uptime</li>
				<li>Published Messages</li>
				<li><input type="button" value="Reboot" ></li>
			</ul>
            <table class="table table-striped">
                  <tr>
                        <td>Broker Address</td>
                        <td><input type="text" name="broker" value="{{broker}}" onchange="checkEmpty(this, event)"></td>
                  </tr>
                  <tr>
                        <td>Broker Port</td>
                        <td><input type="number" name="port" min="1" max="65535" step="1"  value="{{port}}"></td>
                  </tr>
                  <tr>
                        <td>Client ID</td>
                        <td><input type="text" name="client" value="{{client}}" onchange="checkEmpty(this, event)"></td>
                  </tr>
                  <tr>
                        <td>Username</td>
                        <td><input type="text" name="user" value="{{user}}"></td>
                  </tr>
                  <tr>
                        <td>Password</td>
                        <td><input type="password" name="password" id="pw1" value="{{password}}" onchange="checkPwd(event)"></td>
                  </tr>
                  <tr>
                        <td>Repeat Password</td>
                        <td><input type="password" name="password2" id="pw2" value="{{password}}" onchange="checkPwd(event)"><span id="pwerr"></span></td>
                  </tr>
                  <tr>
                        <td>Last Will Active</td>
                        <td><input type="checkbox" name="will" id="will" onchange="processWill(event)"></td>
                  </tr>
                  <tr>
                        <td>Will Topic</td>
                        <td><input type="text" name="willtopic" id="wtopic" value="{{wtopic}}" onchange="prevent(event)"></td>
                  </tr>
                  <tr>
                        <td>Will Message</td>
                        <td><input type="text" name="willmessage" id="wmessage" value="{{wmessage}}" onchange="prevent(event)"></td>
                  </tr>
            </table>
`;


function prevent( event ) {
      event.stopPropagation();
}

function checkEmpty( element, event ) {
      if( element.value == '' ){
            element.value = 'must not be empty';
            event.stopPropagation();
      }
}

function checkPwd(event) {
	if($('#pw1').prop('value') != $('#pw2').prop('value')) {
		$('#pwerr').text('passwords unidentical');
		event.stopPropagation();
	}
	else
		$('#pwerr').text('');
}

// Used by the Expand2Click output bits
// Used by the Expand2Click output bits
function toggleBit( element, event ) {
    var x = 0;
    $($('input[name="'+element.name+'"').get().reverse()).each(function(i, v) {
        $(v).prop('checked') && ( x += 2**i );
    });
    // Change the numeric input field, which triggers the request to the GreenPHY module.
    $('input[name="'+element.name.substring(0, element.name.lastIndexOf('['))+'"]').val(x).change();
    // Prevent triggering the request twice.
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
            $('#hostname').text(json['hostname']);
            json['uptime'] += ' s';
            json['free_heap'] += ' B';
            break;
        case 'config':
            $('#nav .clickboard').addClass('hidden');
            $.each(json['clickboards'], function(i, clickboard) {
                clickboard['name_format'] = capitalize(clickboard['name']) + 'Click';
                clickboard['port1_available'] = clickboard['available'] & (1 << 0) ? true : false;
                clickboard['port2_available'] = clickboard['available'] & (1 << 1) ? true : false;
                clickboard['port1_active'] = clickboard['active'] & (1 << 0) ? true : false;
                clickboard['port2_active'] = clickboard['active'] & (1 << 1) ? true : false;

                // If the clickboard is active, add an entry to the menu.
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
    // Set domain for testing purposes to GreenPHY module URL like 'http://172.16.201.3/'
    var domain = '';
    if( !data ) data = { action: 'get' };
    $.getJSON(domain + page + '.json', data, success)
            .fail(function(xhr, text_status, error_thrown) {
                    console.log(text_status);
                    console.log(error_thrown);
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
        $('#content').html(html);
        // Mark navigation link as active
        $('#nav a.active').removeClass('active');
        $('#nav a[href^="#' + page + '"]').addClass('active');

        // Change title
        $('#page-title').html($('#nav a.active').html());
    }
}

function setRefreshRate( rate ) {
    $('input[name="refresh"]').val(rate).trigger('input');
}

function getRefreshRate() {
    var rates = [0,0.5, 1, 3, 5, 10, 20, 30, 60];
    return rates[$('input[name="refresh"]').val()];
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
		if( timeout ) clearTimeout(timeout);
        if(getRefreshRate() != 0)
            timeout = setTimeout(updatePage, getRefreshRate()*1000);
    });
}

function processWill( event ) {
      event.stopPropagation();
      if( $('#will').prop( 'checked' ) ) {
            if(( $('#wtopic').prop( 'value' ) != '' ) && ( $('#wmessage').prop( 'value' ) != '' )) {
                  var str = 'will=1,willtopic=' + $('#wtopic').prop( 'value' ) + ',willmessage=' + $('#wmessage').prop( 'value' );
                  updatePage( undefined, str );
            }
            else {
                  $('#will').prop( 'checked', false );
            }
      }
      else {
            var str = 'will=0';
            updatePage( undefined, str );
      }
}

// Handle internal links by JQuery
$(document).on('click', 'a[href^="#"]', function(event) {
    event.preventDefault();
    updatePage(this.hash.substr(1));
});

function serialize(element) {
    var data = $(element).serialize();
    if( $(element).is(':checkbox') && !element.checked ) {
        data += element.name + '=off';
    }
    return data;
}

// Submit input fields on change
$(document).on('change', 'input, select', function() {
    updatePage(undefined, serialize(this));
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
    sendRequest('config', 'write', $.noop );
});

// Send request to erase config from flash
$('#erase-config').click(function() {
    sendRequest('config', 'erase', $.noop );
});

// Send request to reset system
$('#reset-system').click(function() {
    $.getJSON('status.json', 'reset');
});
