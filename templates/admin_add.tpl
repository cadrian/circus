<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <h1>New user created: {{form:username}}</h1>
    <table>
      <tr>
        <td>
          <b>Password:</b>
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
    <form method="POST" name="action_form" action="{{cgi:script_name}}/admin_add.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action" value="admin_add"/>
      <input type="submit" value="Back"/>
    </form>
  </body>
</html>
