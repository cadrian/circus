<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <h1>Welcome to Circus</h1>
    <p>Please log in.</p>
    <form method="POST" action="{{cgi:script_name}}/login.do">
      <table>
        <tr>
          <td>
            <b>User name:</b>
          </td>
          <td>
            <input type="text" name="userid"/>
          </td>
        </tr>
        <tr>
          <td>
            <b>Password:</b>
          </td>
          <td>
            <input type="password" name="password"/>
          </td>
        </tr>
        <tr>
          <td colspan="2">
            <input type="submit" name="action" value="ok"/>
          </td>
        </tr>
      </table>
    </form>
  </body>
</html>
