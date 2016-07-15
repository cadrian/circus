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
        <h2>Your account</h2>
        <p>The password has been changed.</p>
        <table>
          <tr>
            <td>
              <b>Password validity:</b>
            </td>
            <td>
              {{validity}}
              {{^validity}}
              infinite.
              {{/validity}}
            </td>
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
