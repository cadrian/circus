<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <h1>Circus: pass</h1>
    <form method="POST" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action" value="list"/>
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
    </form>
  </body>
</html>
