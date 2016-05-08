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
        {{#names}}
        <tr>
          <td>
            Show password:
          </td>
          <td>
            <button type="button" onclick="get_pass("{{item}}")">{{item}}</button><br/>
          </td>
        </tr>
        {{/names}}
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
          <td rowspan="3">
            Set password:
          </td>
          <td>
            <input type="text" name="key"/>
          </td>
        </tr>
        <tr>
          <td>
            <table>
              <tr>
                <td>
                  Recipe:
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
            </table>
          </td>
        </tr>
        <tr>
          <td>
            <table>
              <tr>
                <td>
                  Password:
                </td>
                <td>
                  <input type="password" name="pass1"/>
                </td>
              </tr>
              <tr>
                <td>
                  Again:
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
      </table>
    </form>
  </body>
</html>
