<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
    <script type="text/javascript" src="{{config:static_path}}/circus.js"></script>
  </head>
  <body>
    <form method="POST" name="action_form" action="{{cgi:script_name}}/admin_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action"/>
      <div id="navigation">
      </div>
      <div id="centerDoc">
        <h1>Circus: Ring-Master</h1>
        <h2>Add a user</h2>
        <p>Please enter the id of a user to create or update.</p>
        <table>
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
                <!-- option value="admin">Ring-Master</option -->
                <option value="user" select="true">Clown</option>
              </select>
            </td>
          </tr>
          <tr>
            <td colspan="2">
              <button type="button" onclick="submit_action('add_user');">Create user</button>
              |
              <button type="button" onclick="submit_action('logout');">Disconnect</button>
            </td>
          </tr>
        </table>
      </div>
    </form>
  </body>
</html>
