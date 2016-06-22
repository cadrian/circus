<html>
  <head>
    <title>Circus</title>
    <script>
      function submit_action(action) {
          document.action_form.action.value = action;
          document.action_form.submit();
      }
    </script>
  </head>
  <body>
    <h1>Welcome to Circus</h1>
    <form method="POST" name="action_form" action="{{cgi:script_name}}/admin_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action"/>
      <table>
        <tr>
          <td colspan="2">
            <h2>Add a user</h2>
          </td>
        </tr>
        <tr>
          <td>
            <b>User name:</b>
          </td>
          <td>
            <input type="test" name="username"/>
          </td>
        </tr>
        <tr>
          <td>
            <b>E-mail:</b>
          </td>
          <td>
            <input type="text" name="email"/>
          </td>
        </tr>
        <tr>
          <td>
            <b>Permissions:</b>
          </td>
          <td>
            <select name="permissions">
              <!-- option value="admin">Administrator</option -->
              <option value="user" select="true">User</option>
            </select>
          </td>
        </tr>
        <tr>
          <td colspan="2">
            <button type="button" onclick="submit_action('add_user');">Create user</button>
            <button type="button" onclick="submit_action('logout');">Disconnect</button>
          </td>
        </tr>
      </table>
    </form>
  </body>
</html>
