<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <form method="POST" name="action_form" action="{{cgi:script_name}}/admin_add.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action" value="admin_add"/>
      <div id="navigation">
      </div>
      <div id="centerDoc">
        <h1>Circus: Ring-Master</h1>
        <h2>New user created</h2>
        <p>The user was successfully created: {{form:username}}</p>
        <p>Please send them their temporary credentials:</p>
        <table>
          <tr>
            <td>
              <b>Ticket:</b>
            </td>
            <td>
              {{password}}
            </td>
          </tr>
          <tr>
            <td>
              <b>Validity:</b>
            </td>
            <td>
              {{validity}}
            </td>
          </tr>
        </table>
        <input type="submit" value="Back"/>
      </div>
    </form>
  </body>
</html>
