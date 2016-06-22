<html>
  <head>
    <title>Circus</title>
  </head>
  <body>
    <h1>Circus: change user password</h1>
    <form method="POST" action="{{cgi:script_name}}/user_home.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <table>
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
            <input type="submit" name="action" value="chpwd"/>
          </td>
        </tr>
      </table>
    </form>
  </body>
</html>
