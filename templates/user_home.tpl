<html>
  <head>
    <title>Circus</title>
    <script>
      function submit_action(action) {
          document.action_form.action.value = action;
          document.action_form.submit();
      }
      function get_pass(key) {
          document.action_form.name = key;
          submit_action('pass');
      }
    </script>
  </head>
  <body>
    <h1>Welcome to Circus</h1>
    <form method="POST" name="action_form" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action"/>
      <input type="hidden" name="name"/>

      <table>

        <tr>
          <td colspan="2">
            <h2>Your passwords</h2>
          </td>
        </tr>

        {{#names.count}}
        <tr>
          <td rowspan="{{names.count}}">
            <h2>Show password:</h2>
          </td>
          <td>
            {{#names}}
            <button type="button" onclick="get_pass('{{item}}')">{{item}}</button><br/>
          </td>
        </tr>
        <tr>
          <td>
            {{/names}}
            &nbsp;
          </td>
        </tr>
        {{/names.count}}
        {{^names}}
        <tr>
          <td colspan="2">
            <button type="button" onclick="submit_action('list')">Click to load the passwords list</button><br/>
          </td>
        </tr>
        {{/names}}

        <tr>
          <td colspan="2">
            &nbsp;
          </td>
        </tr>

        <tr>
          <td style="vertical-align:top;">
            <h2>Set password:</h2>
          </td>
          <td>
            <table>
              <tr>
                <td>
                  <b>Name:</b>
                </td>
                <td>
                  <input type="text" name="key"/>
                </td>
              </tr>
              <tr>
                <td colspan="2">
                  &mdash;<i>Either generate using a recipe</i>&mdash;
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
                  &mdash;<i>Or enter the new password</i>&mdash;
                </td>
              </tr>
              <tr>
                <td>
                  <b>Password:</b>
                </td>
                <td>
                  <input type="password" name="pass1"/>
                </td>
              </tr>
              <tr>
                <td>
                  <b>Again:</b>
                </td>
                <td>
                  <input type="password" name="pass2"/>
                </td>
              </tr>
              <tr>
                <td colspan="2">
                  <button type="button" onclick="submit_action('add_prompt')">Define</button>
                </td>
              </tr>
            </table>
          </td>
        </tr>

        <tr>
          <td colspan="2">
            <h2>Your passwords</h2>
          </td>
        </tr>

        <tr>
          <td colspan="2">
            &nbsp;
          </td>
        </tr>

        <tr>
          <td>
            <b>Current password:</b>
          </td>
          <td>
            <input type="password" name="old"/>
          </td>
        </tr>
        <tr>
          <td>
            <b>New password:</b>
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
            <button type="button" onclick="submit_action('password')">Change password</button><br/>
          </td>
        </tr>

        <tr>
          <td>
            <button type="button" onclick="submit_action('logout');">Disconnect</button>
          </td>
        </tr>
      </table>
    </form>
  </body>
</html>
