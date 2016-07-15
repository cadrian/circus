<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <form method="POST" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action" value="list"/>
      <div id="navigation">
      </div>
      <div id="centerDoc">
        <h1>Circus: user</h1>
        <h2>Your password</h2>
        <p>The password is loaded.</p>
        <table>
          <tr>
            <td>
              <b>Key:</b>
            </td>
            <td>
              {{key}}
            </td>
          </tr>
          <tr>
            <td>
              <b>Pass:</b>
            </td>
            <td>
              {{pass}}
            </td> <!-- TODO TO HIDE!!! -->
          </tr>
          <tr>
            <td colspan="2">
              <input type="submit" value="Back"/>
            </td>
          </tr>
        </table>
      </div>
    </form>
  </body>
</html>
