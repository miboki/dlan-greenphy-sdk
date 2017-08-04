var templates = {};
templates['status'] = `
        <h2>Server Status</h2>
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
        <h2>Configuration</h2>
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
        <h2>Color2Click</h2>
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

function toggleLED() {
  	xhr = $.getJSON('status.json?action=set&led=' + ($('#led-switch').is(":checked") ? 'on' : 'off'), function(json) {});
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

function capitalize( string ) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

function processJSON(page, json) {
	switch(page) {
		case 'config':
			$('#nav .clickboard a').attr('href', '#').text('');
			$.each(json['clickboards'], function(i, clickboard) {
				clickboard['name_format'] = capitalize(clickboard['name']) + 'Click';
				clickboard['port1_available'] = clickboard['available'] & (1 << 0) ? true : false;
				clickboard['port2_available'] = clickboard['available'] & (1 << 1) ? true : false;
				clickboard['port1_active'] = clickboard['active'] & (1 << 0) ? true : false;
				clickboard['port2_active'] = clickboard['active'] & (1 << 1) ? true : false;

				/* If the clickboard is active, add an entry to the menu. */
				if(clickboard['port1_active']) {
					$('#nav .clickboard a').eq(0).attr('href', '#'+clickboard['name']).text(clickboard['name_format']);
				}
				if(clickboard['port2_active']) {
					$('#nav .clickboard a').eq(1).attr('href', '#'+clickboard['name']).text(clickboard['name_format']);
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
		default:
			break;
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
        	$('#content').fadeOut(100, function() {
            	$('#content').html(html).fadeIn(200);
        	});
      	}

      	currentPage = page;
}

function switchPage() {
  	var page = window.location.hash.substr(1);
  	if( !(page in templates) ) {
		page = 'status';
  	}

  	$.getJSON(page + '.json?action=get', function(json) {
  		renderPage(page, json);
  	});
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
}

$("a[href^='#']").click(link);
$(document).keydown(keydown);

$.getJSON('config.json?action=get', function(json) {
	processJSON('config', json);
});
switchPage();
