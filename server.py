from flask import Flask, request, jsonify
import pandas as pd
import numpy as np
import cv2
import dlib
import os
import subprocess
from flask_cors import CORS

# Load data
students_df = pd.read_csv('executable/data/students.csv')
attendance_df = pd.read_csv('executable/data/attendance.csv')

# Ensure the executable path
executable_path = './executable/create_avl'  # Path to your compiled C++ executable
output_folder = 'executable/serialized/'
trie_executable = './executable/create_trie'
vector_distance = './executable/distance'

# Ensure the output folder exists
os.makedirs(output_folder, exist_ok=True)

# Function to create AVL tree for each subject
def create_avl_tree_for_subject(subject):
    # Drop NaN values for the subject
    subject_data = attendance_df[['student_id', subject]].dropna()

    # Path to save the .dat file
    serialized_filename = os.path.join(output_folder, f"{subject}.dat")

    # Command to run the executable with the .dat path as argument
    command = [executable_path, serialized_filename]

    # Prepare input string: "attendance student_id"
    input_lines = [f"{row[subject]} {row['student_id']}\n" for _, row in subject_data.iterrows()]
    input_str = ''.join(input_lines)

    try:
        # Run the executable and feed the input string via stdin
        result = subprocess.run(command, input=input_str.encode(), check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"[✓] AVL tree for '{subject}' saved as '{serialized_filename}'")
    except subprocess.CalledProcessError as e:
        print(f"[✗] Error creating AVL for '{subject}': {e.stderr.decode()}")
    except Exception as e:
        print(f"[✗] Unexpected error: {str(e)}")

# Iterate over the subjects and create AVL trees
subjects = ['maths', 'english', 'chemistry', 'physics', 'datastructure', 'total_attendance']
for subject in subjects:
    create_avl_tree_for_subject(subject)

# Now call the create_trie executable
try:
    result = subprocess.run([trie_executable], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    print("[✓] Trie creation completed successfully.")
except subprocess.CalledProcessError as e:
    print(f"[✗] Error running create_trie: {e.stderr.decode()}")
except Exception as e:
    print(f"[✗] Unexpected error running create_trie: {str(e)}")

app = Flask(__name__)
CORS(app)

# Load dlib models
detector = dlib.get_frontal_face_detector()
sp = dlib.shape_predictor("dat/shape_predictor_68_face_landmarks.dat")
face_rec_model = dlib.face_recognition_model_v1("dat/dlib_face_recognition_resnet_model_v1.dat")

def capture_face_vector():
    """Capture an image from webcam and return the average face encoding vector after different poses"""
    instructions = [
        "Look Straight",
        "Look Left",
        "Look Right"
    ]
    
    vectors = []
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        raise Exception("Could not open video device")

    for instruction in instructions:
        print(f"[INFO] {instruction} and hold still...")
        
        success = False
        frame = None
        
        while not success:
            ret, frame = cap.read()
            if not ret:
                continue

            # Display instructions
            display_frame = frame.copy()
            cv2.putText(display_frame, instruction, (30, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.imshow("Face Registration - Press SPACE to capture", display_frame)

            key = cv2.waitKey(1) & 0xFF
            if key == 32:  # SPACE key
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                faces = detector(gray)
                if faces:
                    landmarks = sp(gray, faces[0])
                    descriptor = face_rec_model.compute_face_descriptor(frame, landmarks)
                    vectors.append(np.array(descriptor))
                    success = True
                    print(f"[OK] {instruction} captured.\n")
                else:
                    print("[WARN] No face detected. Try again.")
    
    cap.release()
    cv2.destroyAllWindows()

    if not vectors:
        raise Exception("Failed to capture any valid face vectors")
    
    avg_vector = np.mean(vectors, axis=0)
    return avg_vector

@app.route('/add_student', methods=['POST'])
def add_student():
    global students_df, attendance_df
    try:
        name = request.form['name']
        rn = request.form['rn']
        student_id = request.form['student_id']

        # Check for uniqueness
        if rn in students_df['rn'].values:
            return jsonify({'status': 'error', 'message': 'Roll number already exists'}), 400
        if student_id in students_df.index:
            return jsonify({'status': 'error', 'message': 'Student ID already exists'}), 400

        # Capture facial vector
        face_vector = capture_face_vector()

        # Convert the face vector to a comma-separated string
        face_vector_str = ','.join(map(str, face_vector))

        # Append to students_df
        new_student_row = pd.DataFrame({
            'student_id':[student_id],
            'name': [name],
            'rn': [rn],
            'facial_vector': [face_vector_str]  # Store as comma-separated string
        })
        students_df = pd.concat([students_df, new_student_row])
        students_df.to_csv('executable/data/students.csv', mode='w', index=False)
        try: 
            subprocess.run(
                ["./executable/insert_trie",name, student_id],
                check=True
            )
            print(f"Student {name} added to Trie.")
        except subprocess.CalledProcessError as e:
            return jsonify({"error": "Failed to insert into Trie", "details": e.stderr}), 500
        # Initialize attendance
        new_attendance_row = pd.DataFrame({
            'student_id': [student_id],
            'name': [name],
            'maths': [0],
            'english': [0],
            'chemistry': [0],
            'physics': [0],
            'datastructure': [0],
            'total_attendance': [0]
        })
        attendance_df = pd.concat([attendance_df, new_attendance_row])
        attendance_df.to_csv('executable/data/attendance.csv', mode='w', index=False)
        subject_list = ['maths', 'english', 'chemistry', 'physics', 'datastructure', 'total_attendance']
        for subject in subject_list:  # e.g., ['Math', 'Science']
            subprocess.run(
                ["./executable/update_avl", f"./executable/serialized/{subject}.dat", 
                "0", str(student_id)],
                check=True
            )
        return jsonify({'status': 'success', 'message': 'Student added successfully'})
        attendance_df = pd.read_csv('executable/data/attendance.csv')
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500
    
@app.route('/verify',methods=['POST'])
def verify():
    global students_df, attendance_df
    try:
        subject = request.form['subject']
        face_vector = capture_face_vector()

        # Convert the face vector to a comma-separated string
        face_vector_str = list(map(str, face_vector))
        command = ['./executable/distance'] + face_vector_str
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, text=True)
        result_str = result.stdout.strip()
        student_id = int(result_str) 
        print("Executable returned integer:", student_id)
        if(student_id != -1):
            attendance_df.loc[attendance_df['student_id'] == student_id, subject] += 1
            attendance_df.loc[attendance_df['student_id'] == student_id, 'total_attendance'] += 1
            attendance_df.to_csv('executable/data/attendance.csv', mode='w', index=False)
            row = attendance_df[attendance_df['student_id'] == student_id]
            if row.empty:
                return jsonify({'status': 'error', 'message': 'Student not found in attendance'}), 404
            attendance_value = row.iloc[0][subject]
            attendance_value1 = row.iloc[0]['total_attendance']
            try:
                subprocess.run(
                    ["./executable/update_avl", f"./executable/serialized/{subject}.dat", 
                    str(attendance_value), str(student_id)],
                    check=True
                )
                subprocess.run(
                    ["./executable/update_avl", f"./executable/serialized/total_attendance.dat", 
                    str(attendance_value1), str(student_id)],
                    check=True
                )
                return jsonify({'status': 'success', 'message': 'Attendance marked successfully'})
            except subprocess.CalledProcessError as e:
                return jsonify({"error": "Failed to update AVL tree", "details": e.stderr}), 500
        else:
            print('No student found')
            return jsonify({'status': 'error', 'message': 'No student found'})
    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/get_attendance', methods=['GET'])
def get_attendance():
    """
    Route to get the entire attendance data from attendance_df
    Returns JSON of all attendance records
    """
    try:
        # Convert DataFrame to a list of dictionaries (records)
        attendance_records = attendance_df.to_dict(orient='records')
        return jsonify({
            'status': 'success',
            'data': attendance_records
        })
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@app.route('/search_students', methods=['POST'])
def search_students():
    """
    Route to search students by name using the trie structure
    Takes a string input, calls the trie executable, and returns attendance for matching students
    """
    try:
        # Get search query from request
        search_query = request.form.get('query', '')
        if not search_query:
            return jsonify({
                'status': 'error',
                'message': 'Search query is required'
            }), 400

        # Call the search_trie executable
        try:
            result = subprocess.run(
                ["./executable/search_trie", search_query],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            student_ids_str = result.stdout.strip()
            
            # If no results found
            if not student_ids_str:
                return jsonify({
                    'status': 'success',
                    'data': []
                })
            
            # Parse student IDs from the output string
            student_ids = [int(id_str) for id_str in student_ids_str.split()]
            
            # Filter attendance data for these student IDs
            matching_records = attendance_df[attendance_df['student_id'].isin(student_ids)].to_dict(orient='records')
            
            return jsonify({
                'status': 'success',
                'data': matching_records
            })
        except subprocess.CalledProcessError as e:
            return jsonify({
                'status': 'error',
                'message': f"Trie search failed: {e.stderr}"
            }), 500
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@app.route('/threshold_attendance', methods=['POST'])
def threshold_attendance():
    """
    Route to get students with attendance above/below a threshold for a specific subject
    Takes a subject, threshold value, and direction, calls the threshold executable,
    and returns attendance for matching students
    """
    try:
        # Get subject, threshold and direction from request
        subject = request.form.get('subject', '')
        threshold = request.form.get('threshold', '')
        direction = request.form.get('direction', '')
        
        # Validate inputs
        if not subject or not threshold or not direction:
            return jsonify({
                'status': 'error',
                'message': 'Subject, threshold and direction are all required'
            }), 400
        
        # Check if subject is valid
        valid_subjects = ['maths', 'english', 'chemistry', 'physics', 'datastructure', 'total_attendance']
        if subject not in valid_subjects:
            return jsonify({
                'status': 'error',
                'message': f'Subject must be one of: {", ".join(valid_subjects)}'
            }), 400
        
        try:
            threshold = int(threshold)
        except ValueError:
            return jsonify({
                'status': 'error',
                'message': 'Threshold must be an integer'
            }), 400
            
        if direction not in ['-1', '1']:
            return jsonify({
                'status': 'error',
                'message': 'Direction must be either -1 or 1'
            }), 400
        
        # Call the threshold executable with the subject-specific AVL tree
        try:
            result = subprocess.run(
                ["./executable/threshold", f"./executable/serialized/{subject}.dat", str(threshold), direction],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            student_ids_str = result.stdout.strip()
            
            # If no results found
            if not student_ids_str:
                return jsonify({
                    'status': 'success',
                    'data': []
                })
            
            # Parse student IDs from the output string
            student_ids = [int(id_str) for id_str in student_ids_str.split()]
            
            # Filter attendance data for these student IDs
            matching_records = attendance_df[attendance_df['student_id'].isin(student_ids)].to_dict(orient='records')
            
            return jsonify({
                'status': 'success',
                'data': matching_records
            })
        except subprocess.CalledProcessError as e:
            return jsonify({
                'status': 'error',
                'message': f"Threshold search failed: {e.stderr}"
            }), 500
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

if __name__ == '__main__':
    app.run(debug=True)