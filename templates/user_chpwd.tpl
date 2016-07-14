<html>
  <head>
    <title>Circus</title>
  </head>
  <body>
    <h1>Circus: change user password</h1>
    <form method="POST" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="hidden" name="action" value="list"/>
      <table>
        <tr>
          <td>
            <b>Password validity:</b>
          </td>
          <td>
            {{validity}}
          </td>
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
