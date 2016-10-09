<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
    <script type="text/javascript" src="{{config:static_path}}/circus.js"></script>
  </head>
  <body>
    <form method="POST" name="action_form" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action"/>
      <div id="navigation">
        <ul class="menu">
          <li><a class="tablink" href="#" onclick="tabulate(event, 'passwords-list')">Your Jokers</a></li>
          <li><a class="tablink" href="#" onclick="tabulate(event, 'passwords-form')">Define a Joker</a></li>
          <li><a class="tablink" href="#" onclick="tabulate(event, 'credentials')">Your account</a></li>
        </ul>
      </div>
      <div id="centerDoc">
        <h1>Circus: Clown</h1>

        <div id="passwords-list" class="tab">
          <h2>Your Jokers</h2>
          {{#names.count}}
              <p>You have {{item}} Jokers:</p>
              <ul class="key">
              {{#names}}
              <li><a href="#" onclick="get_pass('{{item}}')">{{item}}</a></li>
              {{/names}}
              </ul>
          {{/names.count}}
          {{^names}}
              The Jokers list is not yet loaded.<br/>
              <button type="button" onclick="submit_action('list')">Click to load the Jokers list</button><br/>
          {{/names}}
        </div>

        <div id="passwords-form" class="tab">
          <h2>Define a Joker</h2>
          <table>
            <tr>
              <td>
                <b>Joker name:</b>
              </td>
              <td>
                <input type="text" name="key"/>
              </td>
            </tr>
            <tr>
              <td colspan="2">
                &mdash;<i>Either generate the Joker using a recipe</i>&mdash;
              </td>
            </tr>
            <tr>
              <td>
                <b>Recipe:</b>
              </td>
              <td>
                <input type="text" name="recipe" value="16ans"/>
              </td>
            </tr>
            <tr>
              <td colspan="2">
                <button type="button" onclick="submit_action('add_recipe')">Generate</button>
              </td>
            </tr>
            <tr>
              <td colspan="2">
                &mdash;<i>Or directly enter the new Joker value (twice)</i>&mdash;
              </td>
            </tr>
            <tr>
              <td>
                <b>Value:</b>
              </td>
              <td>
                <input type="password" name="prompt1"/>
              </td>
            </tr>
            <tr>
              <td>
                <b>Again:</b>
              </td>
              <td>
                <input type="password" name="prompt2"/>
              </td>
            </tr>
            <tr>
              <td colspan="2">
                <button type="button" onclick="submit_action('add_prompt')">Define</button>
              </td>
            </tr>
          </table>
        </div>

        <div id="credentials" class="tab">
          <h2>Your account</h2>
          <table>
            <tr>
              <td>
                <b>Current Ticket:</b>
              </td>
              <td>
                <input type="password" name="old"/>
              </td>
            </tr>
            <tr>
              <td>
                <b>New Ticket:</b>
              </td>
              <td>
                <input type="password" name="pass1"/>
              </td>
            </tr>
            <tr>
              <td>
                <b>Confirm:</b>
              </td>
              <td>
                <input type="password" name="pass2"/>
              </td>
            </tr>
            <tr>
              <td colspan="2">
                <button type="button" onclick="submit_action('password')">Change Ticket</button><br/>
              </td>
            </tr>
          </table>
        </div>

        <button type="button" onclick="submit_action('logout');">Disconnect</button>
      </div>
    </form>
  </body>
</html>
