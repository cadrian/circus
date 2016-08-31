<html>
  <head>
    <title>Circus</title>
    <link href="{{config:static_path}}/circus.css" rel="stylesheet" type="text/css">
  </head>
  <body>
    <form method="POST" action="{{cgi:script_name}}/login.do">
      <div id="navigation">
      </div>
      <div id="centerDoc">
        <h1>Welcome to Circus</h1>
        <h2>Enter the Marquee</h2>
        <p>To enter the Marquee, please enter your name and show your Ticket.</p>
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
              <b>Ticket:</b>
            </td>
            <td>
              <input type="password" name="password"/>
            </td>
          </tr>
          <tr>
            <td colspan="2">
              <input type="hidden" name="action" value="ok"/>
              <input type="submit" name="submit" value="Get in"/>
            </td>
          </tr>
        </table>
      </div>
    </form>
  </body>
</html>
