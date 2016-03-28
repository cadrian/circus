<html>
  <head>
    <title>Circus</title>
  </head>
  <body>
    <h1>Circus: pass</h1>
    <form method="POST" action="list.do">
      <p>Key: {{key}}</p>
      <p>Pass: {{pass}}</p> <!-- TODO TO HIDE!!! -->
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="submit" name="action" value="back"/>
    </form>
  </body>
</html>
