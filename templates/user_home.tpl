<html>
  <head>
    <title>Circus</title>
  </head>
  <body>
    <h1>Welcome to Circus</h1>
    <form method="POST" action="list.do">
      <input type="hidden" name="token" value="{{token}}"/>
      <input type="submit" name="action" value="list"/>
      <p>
        Tag: <input type="text" name="tag"/>
        <input type="submit" name="action" value="search"/>
      </p>
    </form>
    <form method="POST" action="setpass.do">
      <p>
        Set password: <input type="text" name="key"/>
        <input type="text" name="recipe" value="16ans"/>
        <input type="submit" name="action" value="setrecipe"/>
      </p>
    </form>
  </body>
</html>
