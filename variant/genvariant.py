"""
Depends on pystache. Install with 'pip install pystache'.
Then do:
    python genvariant.py < variant.gen.cc > variant.cc
    cpp-makeheader < variant.cc > variant.h
"""
from pystache import TemplateSpec, Renderer
import sys

def repeat(text, N):
  """
  Renders 'text' 'N' times. The only variable allowed is 'i', which will be
  replaces with the current iteration number, which runs from 0 through N-1.
  The rendered output from each iteration will be concatenated together
  without any delimiter between.
  """
  output = ''
  renderer = Renderer()
  for i in range(0, N):
    output += renderer.render(text, {'i': i})
  return output

def repeat_and_comma_contact(text, N):
  """
  Renders 'text' 'N' times. The only variable allowed is 'i', which will be
  replaces with the current iteration number, which runs from 0 through N-1.
  The rendered output from each iteration will be concatenated together
  with ", " between them.
  """
  output = []
  renderer = Renderer()
  for i in range(0, N):
    output.append(renderer.render(text, {'i': i}))
  return ', '.join(output)


class Lambdas(TemplateSpec):
  """
  Sets up the lambdas available to the template we're processing
  (variant.gen.cc).
  """
  def __init__(self, N):
    self.N = N

  def repeat(self, text=None):
    return lambda text: repeat(text, self.N)

  def repeat_and_comma_contact(self, text=None):
    return lambda text: repeat_and_comma_contact(text, self.N)

def main():
  text = sys.stdin.read()
  renderer = Renderer()
  N = int(sys.argv[1]) if (len(sys.argv) > 1) else 16
  print renderer.render(text, Lambdas(N))

if __name__ == "__main__":
  # execute only if run as a script
  main()
