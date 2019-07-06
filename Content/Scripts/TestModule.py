#converted for ue4 use from
#https://github.com/tensorflow/docs/blob/master/site/en/tutorials/_index.ipynb

#import tensorflow as tf
import tensorflow as tf
import unreal_engine as ue
import numpy as np
import sys
import random
import math
from time import sleep

#additional includes
#from tensorflow.python.keras import backend as K	#to ensure things work well with multi-threading
#import numpy as np   	#for reshaping input
#import operator      	#used for getting max prediction from 1x10 output array

class Model():
	def __init__(self, num_states, num_actions, batch_size):
		self._num_states = num_states
		self._num_actions = num_actions
		self._batch_size = batch_size
		
		self._states = None
		self._actions = None

		self._logits = None
		self._optimizer = None
		self._var_init = None

		self._define_model()

	def _define_model(self):
		self._states = tf.placeholder(shape=[None, self._num_states], dtype=tf.float32)
		self._q_s_a = tf.placeholder(shape=[None, self._num_actions], dtype=tf.float32)

		fc1 = tf.layers.dense(self._states, 50, activation = tf.nn.relu)
		fc2 = tf.layers.dense(fc1, 50, activation = tf.nn.relu)
		self._logits = tf.layers.dense(fc2, self._num_actions)
		loss = tf.losses.mean_squared_error(self._q_s_a, self._logits)
		self._optimizer = tf.train.AdamOptimizer().minimize(loss)
		self._var_init = tf.global_variables_initializer()

	def predict_one(self, state, sess):
		return sess.run(self._logits, feed_dict = {self._states:state.reshape(1, self._num_states)})

	def predict_batch(self, states, sess):
		return sess.run(self._logits, feed_dict = {self._states:state})
	
	def train_batch(self, sess, x_batch, y_batch):
		sess.run(self._optimizer, feed_dict = {self._states:x_batch, self._q_s_a: y_batch})

class Memory:
	def __init__(self, max_memory):
		self._max_memory = max_memory
		self._samples = []

	def add_sample(self, sample):
		self._samples.append(sample)
		if len(self._samples) > self._max_memory:
			self._samples.pop(0)

	def sample(self, no_samples):
		if no_samples > len(self._samples):
			return random.sample(self._samples, len(self._samples))
		else:
			return random.sample(self._samples, no_samples)

class GameRunner:
	def __init__(self, sess, model, env, memory, max_eps, min_eps, decay, render = True):
		self._sess = sess
		self._env = env
		self._model = model
		self._memory = memory
		self._render = render
		self._max_eps = max_eps
		self._min_eps = min_eps
		self._decay = decay
		self._eps = self._max_eps
		self._steps = 0
		self._reward_store = []
		self._max_x_store = []

	def run(self):
		state = self._env.reset()
		tot_reward = 0
		max_x = -100
		while True:
			if self._render:
				self._env.render()
			
			action = self._choose_action(state)
			next_state, reward, done, info = self._env.step(action)
			if next_state[0] >= 0.1:
				reward += 10
			elif next_state[0] >= 0.25:
				reward += 20
			elif next_state[0] >= 0.5:
				reward += 100
			
			if next_state[0] > max_x:
				max_x = next_state[0]

			if done:
				next_state = None
			
			self._memory.add_sample((state, action, reward, next_state))
			self._replay()

			self._steps += 1
			self._eps = self._min_eps + (self._max_eps - self._min_eps) * math.exp(-self._decay * self._steps)

			state = next_state
			tot_reward += reward

			if done:
				self._reward_store.append(tot_reward)
				self._max_x_store.append(max_x)
				break
	
	def _choose_action(self, state):
		if random.random() < self._eps:
			return random.randint(0, self._model.num_actions - 1)
		else:
			return np.argmax(self._model.predict_one(state, self._sess))

	def _replay(self):
		batch = self._memory.sample(self._model.batch_size)
		states = np.array([val[0] for val in batch])
		next_states = np.array([(np.zeros(self._model.num_states) if val[3] is None else val[3]) for val in batch])

		q_s_a = self._model.predict_batch(states, self._sess)
		q_s_a_d = self._model.predict_batch(next_states, self._sess)

		x = np.zeros((len(batch), self._model.num_states))
		y = np.zeros((len(batch), self._model.num_actions))

		for i, b in enumerate(batch):
			state, action, reward, next_state = b[0], b[1], b[2], b[3]

			current_q = q_s_a[i]
			if next_state is None:
				current_q[action] = reward
			else:
				current_q[action] = reward + GAMMA * np.amax(q_s_a_d[i])
			
			x[i] = state
			y[i] = current_q
		
		self._model.train_batch(self._sess, x, y)

class Main():
	def Setup(self):
		ue.log_warning("Setup")
		#self._model = Model()
		#self._Sesson = tf.Session()
		#self._Game = GameRunner(self._Sesson, )

	def Run(self):
		ue.log_warning("Run")

	def test(self, a):
		ue.log_warning("Test " + str(len(a)) + " " + str(a[random.randrange(0, 1000)]))
