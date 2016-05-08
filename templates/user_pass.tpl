<html>
  <head>
    <title>Circus</title>
  </head>
  <body>
    <h1>Circus: pass</h1>
    <form method="POST" action="{{cgi:script_name}}/list.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <table>
        <tr>
          <td>
            Key:
          </td>
          <td>
            {{key}}
          </td>
        </tr>
        <tr>
          <td>
            Pass:
          </td>
          <td>
            {{pass}}
          </td> <!-- TODO TO HIDE!!! -->
        </tr>
        <tr>
          <td colspan="2">
            <input type="submit" name="action" value="user_pass"/>
          </td>
        </tr>
      </table>
    </form>
  </body>
</html>
