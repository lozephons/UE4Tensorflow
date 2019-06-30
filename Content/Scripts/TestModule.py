#converted for ue4 use from
#https://github.com/tensorflow/docs/blob/master/site/en/tutorials/_index.ipynb

#import tensorflow as tf
import tensorflow as tf
#import unreal_engine as ue
import numpy as np
import sys

#additional includes
#from tensorflow.python.keras import backend as K	#to ensure things work well with multi-threading
#import numpy as np   	#for reshaping input
#import operator      	#used for getting max prediction from 1x10 output array

class TestClass():
	def test(self):
		#ue.log_warning("Test Called")
		# Create 100 phony x, y data points in NumPy, y = x * 0.1 + 0.3
		x_data = np.random.rand(100).astype(np.float32)
		y_data = x_data * 0.1 + 0.3

		# Try to find values for W and b that compute y_data = W * x_data + b
		# (We know that W should be 0.1 and b 0.3, but Tensorflow will
		# figure that out for us.)
		W = tf.Variable(tf.random.uniform([1], -1.0, 1.0))
		b = tf.Variable(tf.zeros([1]))
		y = W * x_data + b

		# Minimize the mean squared errors.
		loss = tf.reduce_mean(tf.square(y - y_data))
		optimizer = tf.compat.v1.train.GradientDescentOptimizer(0.5)
		train = optimizer.minimize(loss)

		# Before starting, initialize the variables.  We will 'run' this first.
		init = tf.compat.v1.global_variables_initializer()

		# Launch the graph.
		sess = tf.compat.v1.InteractiveSession()
		sess.run(init)

		# Fit the line.
		for step in range(201):
			sess.run(train)
			if step % 20 == 0:
		#		ue.log(step, sess.run(W), sess.run(b))
				print(W.eval())
                
		# Learns best fit is W: [0.1], b: [0.3]

		# Close the Session when we're done.
		sess.close()

a = TestClass()
a.test()
print('end')

from tensorflow.keras import layers


model = tf.keras.Sequential([
# Adds a densely-connected layer with 64 units to the model:
layers.Dense(64, activation='relu', input_shape=(32,)),
# Add another:
layers.Dense(64, activation='relu'),
# Add a softmax layer with 10 output units:
layers.Dense(10, activation='softmax')])

model.compile(optimizer=tf.train.AdamOptimizer(0.001),
              loss='categorical_crossentropy',
              metrics=['accuracy'])

def random_one_hot_labels(shape):
  n, n_class = shape
  classes = np.random.randint(0, n_class, n)
  labels = np.zeros((n, n_class))
  labels[np.arange(n), classes] = 1
  return labels

data = np.random.random((1000, 32))
labels = random_one_hot_labels((1000, 10))

model.fit(data, labels, epochs=10, batch_size=32)