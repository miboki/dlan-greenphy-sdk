var templates = {};
templates['status'] = `<h2>Server Status</h2>
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
        </table>`;
templates['config'] = `<h2>Configuration</h2>
        <h3>Clickboards</h3>
        <table class="table table-striped">
          <tr>
            <th>Clickboard</td>
            <th>Slot 1</td>
            <th>Slot 2</td>
          </tr>
          <tr>
            <td>ColorClick</td>
            <td><input type="radio" name="slot1"></td>
            <td><input type="radio" name="slot2"></td>
          </tr>
          <tr>
            <td>Expand2Click</td>
            <td><input type="radio" name="slot1"></td>
            <td><input type="radio" name="slot2"></td>
          </tr>
        </table>
        
        <h3>Relayr</h3>
        <table class="table table-striped">
          <tr>
            <td><b>Server:</b></td>
            <td>relayr.com</td>
          </tr>
          <tr>
            <td><b>Username:</b></td>
            <td>greenphy</td>
          </tr>
          <tr>
            <td><b>Password:</b></td>
            <td>eval_board2</td>
          </tr>
          <tr>
            <td><b>LED state:</b></td>
            <td id="led">
              <div class="onoffswitch">
                <input type="checkbox" name="onoffswitch" class="onoffswitch-checkbox" id="led-switch" onclick="gpio()">
                <label class="onoffswitch-label" for="led-switch">
                  <span class="onoffswitch-inner"></span>
                  <span class="onoffswitch-switch"></span>
                </label>
              </div>
            </td>
          </tr>
        </table>`;

function toggleLED() {
  xhr = $.getJSON('status.json?action=set&led=' + ($('#led-switch').is(":checked") ? 'on' : 'off'), function(json) {
  })
}

var currentPage;

function switchPage() {
  var page = window.location.hash.substr(1);
  if( !(page in templates) ) {
    page = 'status';
  }

  $.getJSON(page + '.json?action=get', function(json) {
      var html = Mustache.render(templates[page], json.data);

      if( currentPage == page ) {
        $('#content').html(html);
      } else {
        $('#content').fadeOut(100, function() {
            $('#content').html(html).fadeIn(200);
        });
      }

      currentPage = page;
  });
}

$("a[href^='#']").click(function(e) {
    e.preventDefault();
    window.location.hash = this.hash;
    switchPage();
});

switchPage();
