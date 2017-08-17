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
                <input type="checkbox" name="onoffswitch" class="onoffswitch-checkbox" id="led-switch" onclick="toggleLED()" {{#led}}checked{{/led}}>
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
        <form id="config_form" action="config.json" method="get" onsubmit="configSubmit()">
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
            <td><input type="radio" name="port1" value="{{name}}" {{#port1_active}}checked="checked"{{/port1_active}} {{^port1_available}}disabled="disabled"{{/port1_available}} onchange="configSubmit(this)"></td>
            <td><input type="radio" name="port2" value="{{name}}" {{#port2_active}}checked="checked"{{/port2_active}} {{^port2_available}}disabled="disabled"{{/port2_available}} onchange="configSubmit(this)"></td>
          </tr>
          {{/clickboards}}
        </table>
        </form>
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
        <div style="width:200px; height:100px;margin:auto; background:rgb({{r_dec}},{{g_dec}},{{b_dec}});"></div>
`;
// Temolate for thermo3 shows two lists one with the actial values red from the eval Board Memory one one from stored values
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
`;

// Template for Expand 2 Click just shows the Value of the Register for now
templates['expand2'] = `
        <h3>Water Meter</h3>
		<p>
		<form id="expand_form_wmeter" action="expand2.json" method="get">
			<table class="table table-striped"> 
				<tr>
					<td>Water Meter 1</td>
					<td>{{quantity1}}&nbsp;Liter</td>
					<td>
						<select id="wmeter1Port">
						{{#options1}}
							<option value="{{val}}" {{#sel}}selected{{/sel}}>{{name}}</option>
						{{/options1}}
						</select>
					</td>
				</tr>
				<tr>
					<td>Water Meter 2</td>
					<td>{{quantity2}}&nbsp;Liter</td>
					<td>
						<select id="wmeter2Port" value="{{wmeterPort2}}">
						{{#options2}}
							<option value="{{val}}" {{#sel}}selected{{/sel}}>{{name}}</option>
						{{/options2}}
						</select>
					</td>
				</tr>
				<tr>
					<td>Multiplicator</td>
					<td></td>
					<td><input type="text" id="waterMultInput" maxlength="4" value="{{waterMultiplicator}}"></td>
				</tr>
				<tr>
					<td><input type="button" value="Set Ports" onclick="expand2Wmeter()"></td>
					<td></td>
					<td><input type="button" value="Aktualisieren" onclick="expand2akt()"></td>
			</table>
		</form>
		</p>
		<br />
		<br />
		<h3>Input Register</h3>
			<table class="table table-striped">
				<tr>
					<td>Value</td>
					<td>{{input}}</td>
				</tr>
				<tr>
					<td>Bits</td>
					<td>
						{{#inputs}}
						<input type="checkbox" disabled name="{{name}}" {{#val}}checked{{/val}}>
						{{/inputs}}
					</td>
				</tr>
			</table>
		<h3>Output Register</h3>
		<form id="expand_form_out" action="expand2.json" method="get">
			<table class="table table-striped">
				<tr>
					<td>Value</td>
					<td><input type="number" id="outVal" min="0" max="255" value={{output}}></td>
					<td><input type="button" value="Set Output Ports" onclick="expand2Out()">
				</tr>
				<tr>
					<td>Bits</td>
					<td>
						{{#outputs}}
						<input type="checkbox" disabled name="{{name}}" {{#val}}checked{{/val}}>
						{{/outputs}}
					</td>
					<td></td>
				</tr>
			</table>
		</form>
`;

function toggleLED() {
      xhr = $.getJSON('status.json?action=set&led=' + ($('#led-switch').is(":checked") ? 'on' : 'off'), function(json) {});
}

/*******************************************
** Global used Variables
*******************************************/
var lastUptime = 0;
var module_StartupTime;
//var assumedUptime;
var timerAlive = setInterval( function() { askAlive(); } , 5000 );


/*******************************************
** Ask if GreenPHY Module is still alive
** Easiest version: get status.json, so no new EventHandler is needed
*******************************************/
function askAlive() {
	$.getJSON( 'status.json?action=get', function(json) {
		if ( lastUptime == 0 ) {
			module_StartupTime = new Date();
			module_StartupTime.setTime( module_StartupTime.getTime() - ( parseInt( json['uptime'] ) * 1000 ) );
		}
        lastUptime = parseInt( json['uptime'] );
		if ( currentPage == 'status' ) {
			renderPage( 'status', json );
		}
    });
}

function configSubmit( e ) {
    /* A clickboard can be active on only one port at a time. */
    $('#config_form [value='+e.value+']:checked').not(e).each(function() {
          $('#config_form [name='+this.name+'][value=none]').prop('checked', true);
      });
    /* Send the form to the GreenPHY Module via GET request. */
    $.getJSON($('#config_form').attr('action'), $('#config_form').serialize(), function(json) {
            renderPage('config', json);
    });
}

var wmeterport1 = 0;
var wmeterport2 = 0;
var waterMult = 0.25;

function expand2Wmeter( e ) {
	wmeterport1 = $( '#wmeter1Port' ).val();
	wmeterport2 = $( '#wmeter2Port' ).val();
	
	if( parseFloat( $( '#waterMultInput' ).val() ) != 0 )
	waterMult = parseFloat( $( '#waterMultInput' ).val() );
	
if(( wmeterport1 != "0" ) && ( wmeterport1 == wmeterport2 ) ) {
		alert("Both watermeters could ot share one Port! Please select different Ports!");
	}else{
		var expandObj = new Object();
		expandObj.selToggelBits1 = parseInt( wmeterport1 );
		expandObj.selToggelBits2 = parseInt( wmeterport2 );
		$.getJSON( $('#expand_form_wmeter').attr('action'), expandObj, function(json) {
			renderPage('expand2', json);
		});
	}
}


function expand2Out( e ) {
	var expandObj = new Object();
	expandObj.setOut = $( '#outVal' ).val();
	$.getJSON( $('#expand_form_out').attr('action'), expandObj, function(json) {
		renderPage('expand2', json);
	});
}

function expand2akt( e ) {
	switchPage();
}



function capitalize(string) {
	return string.charAt(0).toUpperCase() + string.slice(1);
}


// temp_history stores the past temperature values
var temp_history = [];
// timerTemp is needed to stop stop the switch Page function from repeating every 60 seconds
var timerTemp;
var highTemp = -100;
var highTemp_Time = 0;
var lowTemp = 100;
var lowTemp_Time = 0;

function processJSON(page, json) {
    switch(page) {
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
			var date = new Date();
            // Reload page every 10 seconds
            timerTemp = setInterval( function() { switchPage(); } , 10000 );
			
			// Set current Time, division by 100 is because all Tempvals are stored as Integer intern
            json['cur'] = json['temp_cur'] / 100;
			
			// Check if new high Temp and forward the actual high Value and Time
			if ( highTemp <= ( json['temp_high'] / 100 ) ){
				highTemp_Time = lastUptime;
				highTemp = json['temp_high'] / 100;
			}
            json['high'] = highTemp;
			// time is in seconds since startup
			json['highTime'] = ( lastUptime - highTemp_Time );
			
			// Check if new low Temp and forward the actual low Value and Time
			if ( lowTemp >= ( json['temp_low'] / 100 ) ){
				lowTemp_Time = lastUptime;
				lowTemp = json['temp_low'] / 100;
			}
            json['low'] = lowTemp;
			json['lowTime'] = ( lastUptime - lowTemp_Time );
			
            // Store temp value in the global history
            temp_history.push({ 'date' : date.toLocaleString('de-DE'), 'val' : json['cur'] });
			json['history'] = temp_history;
            break;
        case 'expand2':
            // Reload Page every second
            // timerTemp = setInterval( function() { switchPage(); } , 60000 );
			var namen = [ 'PA0', 'PA1', 'PA2', 'PA3', 'PA4', 'PA5', 'PA6', 'PA7' ];
            json['inputs'] = [];
            json['outputs'] = [];
            for( i = 0; i < 8; i++ ) {
                json['inputs'].unshift({'name':'input'+i, 'val': (json['input'] & ( 1 << i )) });
                json['outputs'].unshift({'name':'output'+i, 'val': (json['output'] & ( 1 << i )) });
            }
            // Convert toggle count to water quantity
            json['quantity1'] = json['count1'] * waterMult;
			json['quantity2'] = json['count2'] * waterMult;
			json['waterMultiplicator'] = waterMult;
			
			json['options1'] = [];
			json['options2'] = [];
			
			json['options1'].unshift({ 'val': 0, 'sel': ( wmeterport1 == 0 ? 1 : 0 ), 'name': 'Off' });
			json['options2'].unshift({ 'val': 0, 'sel': ( wmeterport2 == 0 ? 1 : 0 ), 'name': 'Off' });
			
			for( i = 0; i < 8; i++) {
				json['options1'].unshift({ 'val': Math.pow(2, i) , 'sel': ( wmeterport1 & ( 1 << i )), 'name': namen[i] });
				json['options2'].unshift({ 'val': Math.pow(2, i) , 'sel': ( wmeterport2 & ( 1 << i )), 'name': namen[i] });
			}
    }
    return json;
}

/* Store the currently visible page for page switch animation. */
var currentPage;

function renderPage(page, json) {
	var html = Mustache.render(templates[page], processJSON(page, json));

	if( currentPage == page ) {
		$('#content').html(html);
	} else {
		currentPage = page;
		$('#content').fadeOut(100, function() {
			$('#content').html(html).fadeIn(200);
		});

		// Mark navigation link as active
		$('#nav a.active').removeClass('active');
		$('#nav a[href^="#'+currentPage+'"]').addClass('active');

		// Change title
		$('#page-title').find('span').text($('#nav a.active span').text());
		switch ( page ) {
			case 'status':
				$('#hostname').text(json['hostname']).text();
				$('#page-title').find('svg').removeClass('hidden').find('use').attr('xlink:href', '#icon-info');
				break;
			case 'config':
				$('#page-title').find('svg').removeClass('hidden').find('use').attr('xlink:href', '#icon-cogs');
				break;
			default:
				$('#page-title').find('svg').removeClass('hidden').find('use').attr('xlink:href', '#icon-tree');
				break;
		}
	}
}

function switchPage() {
      var page = window.location.hash.substr(1);
      if( !(page in templates) ) {
        page = 'status';
      }

      $.getJSON(page + '.json?action=get', function(json) {
          renderPage(page, json);
      });
      //clear timer each time switch page is called, otherwise there will be chaos
      clearInterval(timerTemp);
}

function link(e) {
    e.preventDefault();
    window.location.hash = this.hash;
    switchPage();
}

function keydown(e) {
    if (e.ctrlKey && e.keyCode == 82) {
        // 82 = r

        switchPage();

        if (e.preventDefault) {
            e.preventDefault();
        }
        else {
            return false;
        }
    }
	if (e.keyCode == 13){
		e.preventDefault();
	}
}

$('a[href^="#"]').click(link);
$(document).keydown(keydown);

$.getJSON('config.json?action=get', function(json) {
    processJSON('config', json);
});
switchPage();
